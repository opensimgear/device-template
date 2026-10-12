#include "can_client.h"
#include "can_delivery.h"
#include "boot_session.h"
#include <assert.h>
#include <string.h>

static osg_gateway_message_t outgoing;
static unsigned sent; static bool permit=true;
static bool send(void *context,const osg_gateway_frame_t *frame) {
    (void)context; if(!permit) return false; sent++; return osg_gateway_frame_decode(frame,&outgoing);
}
static osg_gateway_frame_t encoded(osg_gateway_message_t m) {
    osg_gateway_frame_t f; assert(osg_gateway_frame_encode(&m,&f)); return f;
}
static osg_gateway_message_t assignment(const osg_can_client_t *c,uint32_t lease) {
    return (osg_gateway_message_t){.type=OSG_GW_ASSIGN,.node=3,.identity=c->identity,
        .boot_session=c->boot,.lease=lease,.gateway_epoch=2};
}
static void client_test(void) {
    osg_gateway_identity_t id={{'O','S','G','D',1}};
    osg_gateway_profile_t p={.button_count=128,.axis_count=8,.hat_count=4,.revision=1};
    for(unsigned i=0;i<8;i++) { p.axis_usage[i]=(uint8_t)(0x30+i); p.axis_neutral[i]=32768; }
    osg_can_client_t c; assert(osg_can_client_init(&c,&id,&p,7,send,NULL));
    uint8_t r[OSG_GW_MAX_REPORT_SIZE]; osg_gateway_profile_neutral(&p,r);
    osg_can_client_tick(&c,0,r); assert(sent==1 && outgoing.type==OSG_GW_ANNOUNCE && outgoing.profile.hat_count==4);
    osg_gateway_frame_t f=encoded(assignment(&c,100));
    assert(!osg_can_client_receive(&c,&f,1)); assert(osg_can_client_active(&c));
    osg_can_client_tick(&c,1,r); assert(outgoing.type==OSG_GW_INPUT && outgoing.sequence==1);
    osg_gateway_message_t ack=outgoing; ack.type=OSG_GW_INPUT_ACK; f=encoded(ack);
    assert(osg_can_client_receive(&c,&f,2)); assert(c.ack_sequence==1);
    assert(!osg_can_client_receive(&c,&f,3)); /* duplicate */
    osg_can_client_tick(&c,11,r); ack=outgoing; ack.type=OSG_GW_INPUT_ACK;
    ack.report[0]^=1; f=encoded(ack); assert(!osg_can_client_receive(&c,&f,12)); /* exact report */
    ack.report[0]^=1; ack.lease--; f=encoded(ack); assert(!osg_can_client_receive(&c,&f,12));
    ack.lease++; ack.boot_session++; f=encoded(ack); assert(!osg_can_client_receive(&c,&f,12));
    ack.boot_session--; f=encoded(ack); assert(osg_can_client_usb(&c,true));
    assert(!osg_can_client_active(&c) && !osg_can_client_receive(&c,&f,13));
    assert(osg_can_client_usb(&c,false)); assert(!osg_can_client_receive(&c,&f,14)); /* cleared history */
    osg_can_client_tick(&c,14,r); ack=outgoing; ack.type=OSG_GW_INPUT_ACK; f=encoded(ack);
    assert(osg_can_client_receive(&c,&f,15));
    uint32_t generation=c.generation; f=encoded(assignment(&c,101));
    assert(!osg_can_client_receive(&c,&f,16) && c.generation!=generation);
    f=encoded(assignment(&c,100)); assert(!osg_can_client_receive(&c,&f,17) && c.lease==101);
    ack.lease=100; f=encoded(ack); assert(!osg_can_client_receive(&c,&f,17));
    permit=false; uint32_t sequence=c.next_sequence; osg_can_client_tick(&c,30,r); assert(c.next_sequence==sequence);
    permit=true; osg_can_client_tick(&c,2016,r); assert(!osg_can_client_active(&c));
    f=encoded(assignment(&c,102)); osg_can_client_receive(&c,&f,2020);
    osg_can_client_tick(&c,2020,r); ack=outgoing; ack.type=OSG_GW_INPUT_ACK; f=encoded(ack);
    assert(!osg_can_client_receive(&c,&f,4020) && !osg_can_client_active(&c)); /* timeout before tick */
    f=encoded(assignment(&c,103)); osg_can_client_receive(&c,&f,4021);
    c.next_sequence=UINT32_MAX; osg_can_client_tick(&c,4021,r); assert(c.exhausted && !osg_can_client_active(&c));
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
