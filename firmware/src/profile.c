#include "profile.h"
#include <string.h>
void profile_neutral(uint8_t report[4]) {
    report[0]=0; report[1]=8; report[2]=0; report[3]=128;
}
void profile_rearm(profile_t *p, const profile_sample_t *s, uint32_t now) {
    osg_button_rearm(&p->button, s->button, now, 10);
    osg_button_rearm(&p->encoder_push, s->encoder_push, now, 10);
    osg_encoder_rearm(&p->encoder, s->encoder_ab);
    for(unsigned i=0;i<4;i++) osg_contact_init(&p->hat[i], (s->hat_contacts>>i)&1, now);
    p->hat_armed=false; p->axis_seen=false; p->axis_available=false;
}
void profile_init(profile_t *p, const profile_sample_t *s, uint32_t now) {
    memset(p,0,sizeof *p); p->axis=32768;
    osg_contact_init(&p->toggle,s->toggle,now);
    osg_contact_init(&p->selector[0],s->selector_low,now);
    osg_contact_init(&p->selector[1],s->selector_high,now);
    profile_rearm(p,s,now);
}
void profile_update(profile_t *p, const profile_sample_t *s, uint32_t now, bool active, uint8_t report[4]) {
    profile_neutral(report);
    if(s->axis_fresh && s->axis_raw<=4095) {
        p->axis=osg_axis_scale(s->axis_raw,0,4095); p->axis_at=now; p->axis_seen=true;
    }
    p->axis_available=p->axis_seen && (uint32_t)(now-p->axis_at)<100;
    bool toggle=osg_contact_update(&p->toggle,s->toggle,now);
    bool low=osg_contact_update(&p->selector[0],s->selector_low,now);
    bool high=osg_contact_update(&p->selector[1],s->selector_high,now);
    p->selector_conflict=low && high;
    uint8_t hat=0; bool ready=true;
    for(unsigned i=0;i<4;i++) {
        if(osg_contact_update(&p->hat[i],(s->hat_contacts>>i)&1,now)) hat|=1U<<i;
        ready=ready && p->hat[i].ready;
    }
    p->hat_conflict=hat!=0 && osg_hat_position(hat)==8;
    if(!active) {
        osg_button_rearm(&p->button,s->button,now,10);
        osg_button_rearm(&p->encoder_push,s->encoder_push,now,10);
        osg_encoder_rearm(&p->encoder,s->encoder_ab); p->hat_armed=false;
        return;
    }
    if(osg_button_update(&p->button,s->button,now)) report[0]|=1;
    if(toggle && p->toggle.ready) report[0]|=2;
    if(osg_button_update(&p->encoder_push,s->encoder_push,now)) report[0]|=16;
    if(p->selector[0].ready && p->selector[1].ready && !p->selector_conflict)
        report[0]|=low ? 32 : high ? 128 : 64;
    if(p->hat_conflict) p->hat_armed=false;
    if(ready && hat==0) p->hat_armed=true;
    if(ready && p->hat_armed) report[1]=osg_hat_position(hat);
    osg_encoder_sample(&p->encoder,s->encoder_ab);
    int8_t direction=osg_encoder_output(&p->encoder,now);
    if(direction>0) report[0]|=4;
    if(direction<0) report[0]|=8;
    uint16_t axis=p->axis_available ? p->axis : 32768;
    report[2]=(uint8_t)axis; report[3]=(uint8_t)(axis>>8);
}
void profile_reported(profile_t *p, const uint8_t report[4], uint32_t now) {
    int8_t direction=(report[0]&4) ? 1 : (report[0]&8) ? -1 : 0;
    osg_encoder_reported(&p->encoder,direction,now);
}
