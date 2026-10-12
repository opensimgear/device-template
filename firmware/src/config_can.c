#include "config_can.h"
#include <stdio.h>
#include <string.h>

static bool route_equal(const osg_gateway_message_t *a,const osg_gateway_message_t *b) {
    return a->node==b->node && a->source_node==b->source_node && a->boot_session==b->boot_session &&
        a->lease==b->lease && a->gateway_epoch==b->gateway_epoch && a->sequence==b->sequence &&
        osg_gateway_identity_equal(&a->identity,&b->identity);
}
static bool live(const osg_module_config_can_t *s) {
    const osg_can_client_t *c=s->client;
    return c && c->node && c->lease && s->route.node==c->node && s->route.source_node==c->gateway_source &&
        s->route.boot_session==c->boot && s->route.lease==c->lease && s->route.gateway_epoch==c->epoch &&
        osg_gateway_identity_equal(&s->route.identity,&c->identity);
}
static bool callback(void *context,const osg_gateway_message_t *m,uint32_t now) {
    return osg_module_config_can_receive(context,m,now);
}
void osg_module_config_can_init(osg_module_config_can_t *s,osg_can_client_t *c,osg_runtime_config_t *r) {
    memset(s,0,sizeof *s); s->client=c; s->runtime=r;
    c->config_receive=callback; c->config_context=s;
}
bool osg_module_config_can_receive(osg_module_config_can_t *s,const osg_gateway_message_t *m,uint32_t now) {
    if(!s || !m || !s->runtime || m->type!=OSG_GW_CONFIG_REQUEST || !m->sequence || !m->config_total ||
       m->config_total>=sizeof s->command || m->config_length>OSG_GW_CONFIG_CHUNK_SIZE || !m->config_length ||
       (unsigned)m->config_offset+m->config_length>m->config_total) return false;
    if((s->assembling || s->responding || s->cached) && !live(s)) {
        if(!s->expected_apply) osg_runtime_invalidate_claim(s->runtime);
        s->expected_apply=false;s->assembling=s->responding=s->cached=false;
    }
    /* A complete request ID retains its immutable reply. Lost replies may be
     * replayed without repeating Apply/Save or other mutations. */
    if(s->cached && route_equal(&s->route,m)) {
        if(m->config_total!=s->total || memcmp(s->command+m->config_offset,m->config_data,m->config_length)) return false;
        if(m->config_offset==0) { s->sent=0; s->responding=true; }
        return true;
    }
    if(s->responding && live(s)) return false;
    if(!s->assembling || !route_equal(&s->route,m)) {
        if(m->config_offset) return false;
        s->route=*m; s->received=0; s->total=m->config_total; s->started=now;
        s->assembling=true; s->cached=false;
    }
    if(!live(s) || m->config_total!=s->total) return false;
    if(m->config_offset<s->received) return (unsigned)m->config_offset+m->config_length<=s->received &&
        !memcmp(s->command+m->config_offset,m->config_data,m->config_length);
    if(m->config_offset!=s->received) return false;
    memcpy(s->command+s->received,m->config_data,m->config_length); s->received+=m->config_length;
    if(s->received!=s->total) return true;
    s->command[s->received]=0; s->assembling=false;
    bool apply=false;
    const char *error=osg_runtime_command(s->runtime,s->command,now,&apply);
    s->expected_apply=apply; /* Preserve owner across the commanded lease renewal. */
    int header=snprintf(s->response,sizeof s->response,error ? "ERR %s\n" : "OK\n",error);
    if(header<0 || (size_t)header>=sizeof s->response) return false;
    size_t json=osg_runtime_snapshot(s->runtime,!strcmp(s->command,"get_active"),s->response+header,sizeof s->response-(size_t)header);
    if(!json) {
        strcpy(s->response,"ERR response_overflow\nnull"); s->response_size=(uint16_t)strlen(s->response);
    } else s->response_size=(uint16_t)((size_t)header+json);
    s->sent=0; s->responding=true; s->cached=true; return true;
}
void osg_module_config_can_tick(osg_module_config_can_t *s,uint32_t now) {
    if(!s || !s->client) return;
    /* Direct CDC uses the same runtime handler and may trigger the expected
     * settings activation without a preceding CAN configuration request. */
    if(s->runtime->pending_apply) s->expected_apply=true;
    if((s->assembling || s->responding || s->cached) && !live(s)) {
        if(!s->expected_apply) osg_runtime_invalidate_claim(s->runtime);
        s->expected_apply=false;
        s->assembling=s->responding=s->cached=false; return;
    }
    if(s->assembling && (uint32_t)(now-s->started)>=4000) s->assembling=false;
    if(!s->responding) return;
    osg_gateway_message_t m=s->route; m.type=OSG_GW_CONFIG_RESPONSE;
    m.config_offset=s->sent; m.config_total=s->response_size;
    unsigned remaining=s->response_size-s->sent;
    m.config_length=(uint8_t)(remaining>OSG_GW_CONFIG_CHUNK_SIZE ? OSG_GW_CONFIG_CHUNK_SIZE : remaining);
    memset(m.config_data,0,sizeof m.config_data); memcpy(m.config_data,s->response+s->sent,m.config_length);
    if(osg_cyphal_send(&s->client->cyphal,&m,now)) {
        s->sent+=m.config_length;
        if(s->sent==s->response_size) s->responding=false;
    }
}
bool osg_module_config_can_busy(const osg_module_config_can_t *s) {
    return s && (s->assembling || s->responding || s->client->cyphal.canard.tx.queue_size);
}
