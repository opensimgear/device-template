#include "can_client.h"
#include <string.h>

static bool route_changed(osg_can_client_t *c) {
    if(c->route_generation==UINT32_MAX) {c->route_exhausted=true;c->exhausted=true;return false;}
    c->route_generation++;return true;
}
static void invalidate(osg_can_client_t *c) {
    memset(c->history,0,sizeof c->history); c->ack_sequence=0;
    c->generation++; c->input_due=true; c->status_due=true;
}
bool osg_can_client_init(osg_can_client_t *c,const osg_gateway_identity_t *id,
    const osg_gateway_profile_t *profile,uint32_t boot,osg_can_send_t send,void *context) {
    if(!c || !osg_gateway_identity_valid(id) || !osg_gateway_profile_valid(profile) || !boot || !send) return false;
    memset(c,0,sizeof *c); c->identity=*id; c->profile=*profile; c->boot=boot;
    c->send=send; c->context=context; c->next_sequence=1;c->configuration_generation=1;c->route_generation=1;
    if(!osg_cyphal_init(&c->cyphal,id,boot,"org.opensimgear.device",send,context)) return false;
    c->random=osg_gateway_crc32(id->bytes,sizeof id->bytes)^boot; if(!c->random) c->random=1; invalidate(c); return true;
}
bool osg_can_client_usb(osg_can_client_t *c,bool direct) {
    if(c->direct==direct) return false;
    c->direct=direct; c->announced=false; c->announce_waiting=false;
    osg_cyphal_reset(&c->cyphal); invalidate(c); return true;
}
void osg_can_client_reset(osg_can_client_t *c) {
    c->node=0; c->gateway_source=0; c->lease=0; c->epoch=0;
    c->announced=false; c->announce_waiting=false;(void)route_changed(c);invalidate(c);
    osg_cyphal_reset(&c->cyphal);
}
bool osg_can_client_active(const osg_can_client_t *c) { return c && c->node && c->lease && !c->direct && !c->exhausted; }
static bool send(osg_can_client_t *c,osg_gateway_message_t *m,uint32_t now) {
    if(!osg_cyphal_send(&c->cyphal,m,now)) { c->tx_dropped++; return false; }
    return true;
}
void osg_can_client_tick(osg_can_client_t *c,uint32_t now,const uint8_t report[OSG_GW_MAX_REPORT_SIZE]) {
    if(c->route_exhausted) return;
    if(c->node && (uint32_t)(now-c->assigned_at)>=OSG_CAN_ASSIGN_TIMEOUT_MS) osg_can_client_reset(c);
    osg_cyphal_tick(&c->cyphal,now);
    if(!c->announce_waiting || (int32_t)(now-c->announce_due)>=0) {
        osg_gateway_message_t m={.type=OSG_GW_ANNOUNCE,.identity=c->identity,.boot_session=c->boot,
            .direct_usb=c->direct,.profile=c->profile,.configuration_generation=c->configuration_generation};
        if(send(c,&m,now)) {
            c->announced=true; c->announced_at=now;
            /* Spread periodic discovery across devices on a shared bus. */
            c->random^=c->random<<13; c->random^=c->random>>17; c->random^=c->random<<5;
            c->announce_due=now+450U+c->random%101U;
        } else c->announce_due=now+20U;
        c->announce_waiting=true;
    }
    /* Route liveness is independent of HID streaming. Disabled controllers
     * and direct-USB devices still acknowledge their assigned CAN lease. */
    if(c->node && c->lease && !c->exhausted && (c->status_due || (uint32_t)(now-c->status_at)>=OSG_CAN_STATUS_MS)) {
        if(c->next_sequence==UINT32_MAX) { c->exhausted=true; invalidate(c); return; }
        osg_gateway_message_t m={.type=OSG_GW_HEARTBEAT,.node=c->node,.identity=c->identity,
            .boot_session=c->boot,.lease=c->lease,.sequence=c->next_sequence,.direct_usb=c->direct};
        if(send(c,&m,now)) { c->next_sequence++; c->status_at=now; c->status_due=false; }
    }
    if(!osg_can_client_active(c) || !c->announced || !report ||
       !osg_gateway_profile_report_valid(&c->profile,report) ||
       (!c->input_due && (uint32_t)(now-c->input_at)<OSG_CAN_INPUT_MS)) return;
    /* Sequences never wrap within a boot. An exhausted boot fails closed. */
    if(c->next_sequence==UINT32_MAX) { c->exhausted=true; invalidate(c); return; }
    osg_gateway_message_t m={.type=OSG_GW_INPUT,.node=c->node,.boot_session=c->boot,
        .lease=c->lease,.sequence=c->next_sequence};
    memcpy(m.report,report,sizeof m.report);
    if(send(c,&m,now)) {
        osg_can_sent_t *h=&c->history[c->history_head]; h->sequence=c->next_sequence++;
        memcpy(h->report,report,sizeof h->report);
        c->history_head=(uint8_t)((c->history_head+1)%OSG_CAN_HISTORY);
        c->input_at=now; c->input_due=false;
    }
}
bool osg_can_client_receive(osg_can_client_t *c,const osg_gateway_frame_t *frame,uint32_t now) {
    if(c->route_exhausted) return false;
    osg_gateway_message_t m;
    uint8_t source=osg_cyphal_node_id(&c->cyphal);
    bool received=osg_cyphal_receive(&c->cyphal,frame,now,&m);
    if(source!=osg_cyphal_node_id(&c->cyphal)) {
        /* The gateway bound its lease to our old transport source. Discover
         * again under the newly selected ID before accepting a fresh route. */
        osg_can_client_reset(c); return false;
    }
    if(!received) return false;
    if(c->node && (uint32_t)(now-c->assigned_at)>=OSG_CAN_ASSIGN_TIMEOUT_MS) osg_can_client_reset(c);
    if(m.boot_session!=c->boot) { c->rejected++; return false; }
    if(m.type==OSG_GW_ASSIGN) {
        if(!osg_gateway_identity_equal(&m.identity,&c->identity) || !m.node || !m.lease || !m.gateway_epoch) {
            c->rejected++; return false;
        }
        /* A live route belongs to one Cyphal gateway source. A competing
         * gateway cannot replace it until reset or assignment expiry. */
        if(c->lease && m.source_node!=c->gateway_source &&
           (uint32_t)(now-c->assigned_at)<OSG_CAN_ASSIGN_TIMEOUT_MS) { c->rejected++; return false; }
        /* Lease ordering is meaningful only within the current gateway epoch.
         * An explicit reset/timeout clears this scope for gateway migration. */
        if(c->epoch==m.gateway_epoch && c->lease && m.lease<c->lease) { c->rejected++; return false; }
        if(c->node!=m.node || c->lease!=m.lease || c->epoch!=m.gateway_epoch || c->gateway_source!=m.source_node) {
            if(!route_changed(c)) {osg_can_client_reset(c);c->rejected++;return false;}
            invalidate(c);
        }
        c->node=m.node; c->lease=m.lease; c->epoch=m.gateway_epoch;
        c->gateway_source=m.source_node; c->assigned_at=now;
        return false;
    }
    if(m.type==OSG_GW_CONFIG_REQUEST) {
        if(!c->node || !c->lease || m.source_node!=c->gateway_source || m.node!=c->node ||
           m.lease!=c->lease || m.gateway_epoch!=c->epoch ||
           !osg_gateway_identity_equal(&m.identity,&c->identity) || !c->config_receive) {
            c->rejected++; return false;
        }
        (void)c->config_receive(c->config_context,&m,now); return false;
    }
    if(m.type!=OSG_GW_INPUT_ACK || !osg_can_client_active(c) || m.source_node!=c->gateway_source ||
       m.node!=c->node || m.lease!=c->lease ||
       !m.sequence || m.sequence<=c->ack_sequence || !osg_gateway_profile_report_valid(&c->profile,m.report)) {
        c->rejected++; return false;
    }
    for(unsigned i=0;i<OSG_CAN_HISTORY;i++) if(c->history[i].sequence==m.sequence) {
        if(memcmp(c->history[i].report,m.report,sizeof m.report)) { c->rejected++; return false; }
        c->ack_sequence=m.sequence; c->ack_at=now; memcpy(c->acknowledged,m.report,sizeof m.report); return true;
    }
    c->rejected++; return false;
}
