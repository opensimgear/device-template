#include "can_client.h"
#include "can_delivery.h"
#include "boot_session.h"
#include <assert.h>
#include <string.h>

typedef struct {
    osg_gateway_frame_t frames[128];
    unsigned head,count;
    bool permit;
} wire_t;
static wire_t device_wire={.permit=true},gateway_wire[2]={{.permit=true},{.permit=true}};
static osg_cyphal_t gateways[2];
static osg_can_client_t client;
static osg_gateway_message_t outgoing[6];
static unsigned sent[6],completions;
static bool send(void *context,const osg_gateway_frame_t *frame) {
    wire_t *wire=context;
    if(!wire->permit) return false;
    assert(wire->count<128);
    wire->frames[(wire->head+wire->count)%128]=*frame; wire->count++; return true;
}
static bool take(wire_t *wire,osg_gateway_frame_t *frame) {
    if(!wire->count) return false;
    *frame=wire->frames[wire->head]; wire->head=(wire->head+1)%128; wire->count--; return true;
}
static void pump(uint32_t now) {
    for(unsigned rounds=0;rounds<128;rounds++) {
        bool progress=false; osg_gateway_frame_t frame;
        while(take(&device_wire,&frame)) {
            progress=true;
            for(unsigned i=0;i<2;i++) {
                osg_gateway_message_t m;
                if(osg_cyphal_receive(&gateways[i],&frame,now,&m) && i==0) {
                    outgoing[m.type]=m; sent[m.type]++;
                }
            }
        }
        for(unsigned i=0;i<2;i++) while(take(&gateway_wire[i],&frame)) {
            progress=true;
            if(osg_can_client_receive(&client,&frame,now)) completions++;
            osg_gateway_message_t ignored;
            (void)osg_cyphal_receive(&gateways[1-i],&frame,now,&ignored);
        }
        if(!progress) return;
    }
    assert(false); /* The bounded transport must not create an endless exchange. */
}
static void tick(uint32_t now,const uint8_t report[OSG_GW_MAX_REPORT_SIZE]) {
    osg_can_client_tick(&client,now,report);
    for(unsigned i=0;i<2;i++) osg_cyphal_tick(&gateways[i],now);
    pump(now);
}
static void emit(unsigned gateway,osg_gateway_message_t message,uint32_t now) {
    assert(osg_cyphal_send(&gateways[gateway],&message,now)); pump(now);
}
static osg_gateway_message_t assignment(uint32_t lease) {
    return (osg_gateway_message_t){.type=OSG_GW_ASSIGN,.node=3,.identity=client.identity,
        .boot_session=client.boot,.lease=lease,.gateway_epoch=2};
}
static osg_gateway_message_t acknowledgement(void) {
    osg_gateway_message_t m=outgoing[OSG_GW_INPUT]; m.type=OSG_GW_INPUT_ACK; return m;
}
static void client_test(void) {
    osg_gateway_identity_t id={{'O','S','G','D',1}};
    osg_gateway_profile_t p={.button_count=128,.axis_count=8,.hat_count=4,.revision=1};
    for(unsigned i=0;i<8;i++) { p.axis_usage[i]=(uint8_t)(0x30+i); p.axis_neutral[i]=32768; }
    for(unsigned i=0;i<2;i++) {
        osg_gateway_identity_t gateway_id={{'O','S','G','G',2}}; gateway_id.bytes[4]+=(uint8_t)i;
        assert(osg_cyphal_init(&gateways[i],&gateway_id,9,"org.opensimgear.gateway",send,&gateway_wire[i]));
        assert(canard_set_node_id(&gateways[i].canard,(uint8_t)(100+i)));
    }
    assert(osg_can_client_init(&client,&id,&p,7,send,&device_wire));
    assert(canard_set_node_id(&client.cyphal.canard,21));
    assert(osg_cyphal_node_id(&client.cyphal)<=127);
    uint8_t r[OSG_GW_MAX_REPORT_SIZE]; osg_gateway_profile_neutral(&p,r);
    tick(0,r); assert(sent[OSG_GW_ANNOUNCE]==1 && outgoing[OSG_GW_ANNOUNCE].profile.hat_count==4);
    emit(0,assignment(100),1); assert(osg_can_client_active(&client));
    assert(client.gateway_source==osg_cyphal_node_id(&gateways[0]));
    tick(1,r); assert(sent[OSG_GW_INPUT]==1 && outgoing[OSG_GW_INPUT].sequence==1);
    osg_gateway_message_t ack=acknowledgement(); emit(0,ack,2);
    assert(completions==1 && client.ack_sequence==1);
    emit(0,ack,3); assert(completions==1); /* Duplicate application sequence. */
    tick(11,r); ack=acknowledgement();
    ack.report[0]^=1; emit(0,ack,12); assert(completions==1); /* Exact report. */
    ack.report[0]^=1; ack.lease--; emit(0,ack,12); assert(completions==1);
    ack.lease++; ack.boot_session++; emit(0,ack,12); assert(completions==1);
    ack.boot_session--; emit(1,ack,12); assert(completions==1); /* Wrong gateway source. */
    emit(1,assignment(999),12); assert(client.lease==100); /* Competing live gateway. */
    assert(osg_can_client_usb(&client,true));
    assert(!osg_can_client_active(&client)); emit(0,ack,13); assert(completions==1);
    assert(osg_can_client_usb(&client,false)); emit(0,ack,14); assert(completions==1); /* Cleared history. */
    tick(14,r); ack=acknowledgement(); emit(0,ack,15); assert(completions==2);
    uint32_t generation=client.generation; emit(0,assignment(101),16);
    assert(client.generation!=generation);
    emit(0,assignment(100),17); assert(client.lease==101);
    ack.lease=100; emit(0,ack,17); assert(completions==2);
    uint8_t old_source=osg_cyphal_node_id(&client.cyphal);
    generation=client.generation;
    assert(canard_set_node_id(&gateways[1].canard,old_source));
    gateways[1].heartbeat_sent=false; osg_cyphal_tick(&gateways[1],18); pump(18);
    assert(osg_cyphal_node_id(&client.cyphal)!=old_source);
    assert(!osg_can_client_active(&client) && !client.lease && !client.ack_sequence && client.generation!=generation);
    unsigned old_inputs=sent[OSG_GW_INPUT],old_announces=sent[OSG_GW_ANNOUNCE];
    tick(19,r); assert(sent[OSG_GW_INPUT]==old_inputs && sent[OSG_GW_ANNOUNCE]==old_announces+1);
    emit(0,assignment(102),20); assert(osg_can_client_active(&client));
    device_wire.permit=false;
    for(unsigned i=0;i<40;i++) { client.input_due=true; tick(30,r); }
    assert(client.tx_dropped>0); /* Driver refusal eventually fills the bounded transport queue. */
    uint32_t sequence=client.next_sequence; tick(30,r); assert(client.next_sequence==sequence);
    osg_can_client_reset(&client); /* Purges retained transfers before restoring the driver. */
    device_wire.permit=true;
    emit(0,assignment(103),2020); tick(2020,r); ack=acknowledgement();
    emit(0,ack,4020); assert(completions==2 && !osg_can_client_active(&client)); /* Timeout before tick. */
    emit(1,assignment(1),4021); assert(client.gateway_source==osg_cyphal_node_id(&gateways[1]));
    /* Timeout cleared epoch/lease ordering, so the new gateway can start at lease 1. */
    client.next_sequence=UINT32_MAX; tick(4021,r); assert(client.exhausted && !osg_can_client_active(&client));
    for(unsigned i=0;i<2;i++) osg_cyphal_destroy(&gateways[i]);
    osg_cyphal_destroy(&client.cyphal);
}
static void encoder_test(void) {
    static const osg_profile_encoder_t enc[]={{0,1}};
    const osg_profile_t p={.name="encoder",.encoders=enc,.encoder_count=1};
    osg_profile_state_t s; osg_profile_sample_t sample={0}; assert(osg_profile_init(&s,&p,&sample,0));
    osg_can_delivery_t d; osg_can_delivery_rearm(&d);
    osg_encoder_t *e=&s.encoders[0]; e->queue[0]=1; e->count=1;
    assert(osg_encoder_output(e,0)==1); osg_can_delivery_observe(&d,&s,10);
    uint8_t report[OSG_GW_MAX_REPORT_SIZE]={1};
    osg_can_delivery_completed(&d,&s,9,report,1); assert(e->phase_at==0 && e->phase==1);
    osg_can_delivery_completed(&d,&s,10,report,2); assert(e->phase_at==2 && e->phase==2);
    assert(osg_encoder_output(e,27)==0); osg_can_delivery_observe(&d,&s,20);
    report[0]=0; osg_can_delivery_completed(&d,&s,19,report,28); assert(e->phase==3);
    osg_can_delivery_completed(&d,&s,20,report,29); assert(e->phase==4);
    e->queue[0]=1; e->head=0; e->count=1; assert(osg_encoder_output(e,54)==1);
    osg_can_delivery_observe(&d,&s,30); report[0]=1;
    osg_can_delivery_completed(&d,&s,10,report,55); assert(e->phase==1); /* old same-direction pulse */
    osg_can_delivery_completed(&d,&s,30,report,56); assert(e->phase==2 && e->phase_at==56);
    osg_can_delivery_rearm(&d); osg_can_delivery_completed(&d,&s,31,report,57); assert(e->phase_at==56);
}
static uint8_t pages[2][OSG_JOURNAL_PAGE_SIZE]; static unsigned operations,fail_at;
static const uint8_t *read_page(void *ctx,unsigned page) { (void)ctx; return pages[page]; }
static bool erase_page(void *ctx,unsigned page) {
    (void)ctx; if(++operations==fail_at) return false; memset(pages[page],0xff,sizeof pages[page]); return true;
}
static bool program(void *ctx,unsigned page,unsigned offset,const uint8_t data[8]) {
    (void)ctx; if(++operations==fail_at) return false;
    for(unsigned i=0;i<8;i++) { assert((pages[page][offset+i]&data[i])==data[i]); pages[page][offset+i]&=data[i]; } return true;
}
static void boot_test(void) {
    osg_journal_io_t io={.read=read_page,.erase=erase_page,.program=program}; uint32_t session;
    memset(pages,0xff,sizeof pages); assert(osg_boot_session_begin(&io,&session) && session==1);
    uint8_t before[sizeof pages]; memcpy(before,pages,sizeof pages);
    operations=0; assert(osg_boot_session_begin(&io,&session) && session==2); unsigned total=operations;
    for(unsigned failure=1;failure<=total;failure++) {
        memcpy(pages,before,sizeof pages); operations=0; fail_at=failure;
        assert(!osg_boot_session_begin(&io,&session));
        fail_at=0; operations=0; assert(osg_boot_session_begin(&io,&session) && session==2);
    }
    memset(pages,0,sizeof pages); assert(!osg_boot_session_begin(&io,&session));
    memset(pages,0xff,sizeof pages); const uint8_t payload[4]={'O','S','G','D'};
    assert(osg_journal_commit(&io,payload,4,UINT32_MAX)); assert(!osg_boot_session_begin(&io,&session));
}
int main(void) { client_test(); encoder_test(); boot_test(); return 0; }
