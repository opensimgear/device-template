#include "profile.h"
#include <assert.h>
#include <stdio.h>
static profile_t p;
static profile_sample_t s;
static uint8_t report[4];
static uint32_t now;
static void tick(void) { profile_update(&p,&s,now++,true,report); }
static void settle(void) { for(unsigned i=0;i<12;i++) tick(); }
static uint16_t axis(void) { return (uint16_t)(report[2]|report[3]<<8); }
static void init(void) {
    now=0; s=(profile_sample_t){.encoder_ab=3}; profile_init(&p,&s,now);settle();
}
static void detent(bool cw) {
    const uint8_t right[]={2,0,1,3},left[]={1,0,2,3};
    for(unsigned i=0;i<4;i++) { s.encoder_ab=cw?right[i]:left[i];tick(); }
}
int main(void) {
    init(); assert(report[0]==64 && report[1]==8 && axis()==32768);
    s.button=true; s.encoder_push=true;settle(); assert((report[0]&17)==17);
    s.button=false; s.encoder_push=false; settle();assert((report[0]&17)==0);
    s.toggle=true; s.selector_low=true;settle();assert(report[0]==34);
    s.selector_high=true;settle();assert(report[0]==2 && p.selector_conflict);
    s.selector_low=false;settle();assert(report[0]==130);
    s.selector_high=false;settle();assert(report[0]==66);
    const uint8_t hats[]={1,3,2,6,4,12,8,9};
    for(unsigned i=0;i<8;i++) {s.hat_contacts=hats[i];settle();assert(report[1]==i);}
    s.hat_contacts=5;settle();assert(report[1]==8 && p.hat_conflict);
    s.hat_contacts=1;settle();assert(report[1]==8);
    s.hat_contacts=0;settle();s.hat_contacts=1;settle();assert(report[1]==0);
    s.axis_fresh=true;s.axis_raw=0;tick();assert(axis()==0 && p.axis_available);
    s.axis_raw=4095;tick();assert(axis()==65535);
    s.axis_raw=2048;tick();assert(axis()==32776);
    s.axis_fresh=false;settle();assert(axis()==32776);
    for(unsigned i=0;i<100;i++) { tick(); }
    assert(axis()==32768 && !p.axis_available);
    s.axis_fresh=true;s.axis_raw=65535;tick();assert(axis()==32768 && !p.axis_available);
    s.axis_raw=0;tick();profile_rearm(&p,&s,now);s.axis_fresh=false;tick();assert(axis()==32768);
    init();detent(true);assert((report[0]&12)==4);
    /* A host stall must hold the press until it has actually completed. */
    for(unsigned i=0;i<80;i++) { tick(); }
    assert((report[0]&12)==4);
    profile_reported(&p,report,now);detent(false);
    for(unsigned i=0;i<26;i++) { tick(); }
    assert((report[0]&12)==0);
    profile_reported(&p,report,now);
    for(unsigned i=0;i<26;i++) { tick(); }
    assert((report[0]&12)==8);
    profile_update(&p,&s,now++,false,report);assert(report[0]==0 && report[1]==8 && axis()==32768);
    settle();assert((report[0]&12)==0);
    /* Startup/recovery with held momentary contacts or hat must not replay actions. */
    now=0;s=(profile_sample_t){.button=true,.encoder_push=true,.hat_contacts=1,.toggle=true,.selector_high=true,.encoder_ab=3};
    profile_init(&p,&s,now);settle();assert(report[0]==130 && report[1]==8);
    s.button=false;s.encoder_push=false;s.hat_contacts=0;settle();
    s.button=true;s.encoder_push=true;s.hat_contacts=1;settle();assert(report[0]==147 && report[1]==0);
    profile_rearm(&p,&s,now);settle();assert(report[0]==130 && report[1]==8);
    puts("Template input behavior passed");
}
