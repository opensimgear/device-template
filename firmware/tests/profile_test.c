#include "profile.h"
#include "can_delivery.h"
#include <assert.h>
#include <string.h>
int main(void) {
    assert(device_profile_prepare());
    const osg_profile_t *profile=device_profile();
    const osg_profile_layout_t *layout=device_report_layout();
    if(!strcmp(profile->name,"Output only")) {
        assert(!layout->length && !layout->buttons && !layout->axes && !layout->hats);
        assert(!layout->digital_mask && !layout->adc_mask && !profile->expander_mask);
        size_t length;device_report_descriptor(&length);assert(!length);
        osg_gateway_profile_t routed;assert(osg_can_profile(profile,layout,&routed) && osg_gateway_profile_valid(&routed));
        uint8_t report[64]={0};assert(!osg_gateway_profile_report_valid(&routed,report));
        osg_profile_sample_t sample={0};osg_profile_state_t state;assert(osg_profile_init(&state,profile,&sample,0));
        osg_profile_update(&state,&sample,100,true,report);return 0;
    }
    if(!strcmp(profile->name,"example")) {
        assert(layout->buttons==8 && layout->axes==1 && layout->hats==1 && layout->length==4);
        assert(layout->digital_mask==0x7ff && layout->adc_mask==1 && !profile->expander_mask);
    } else if(!strcmp(profile->name,"shifter")) {
        assert(layout->buttons==3 && layout->axes==0 && layout->hats==0 && layout->length==1);
        assert(layout->digital_mask==7 && !layout->adc_mask);
    } else if(!strcmp(profile->name,"Master Caution")) {
        assert(layout->buttons==1 && !layout->axes && !layout->hats && layout->length==1);
        assert(layout->digital_mask==1 && !layout->adc_mask);
    } else if(!strcmp(profile->name,"Radio panel")) {
        assert(layout->buttons==7 && layout->axes==1 && !layout->hats && layout->length==3);
        assert(layout->digital_mask==0x7f && layout->adc_mask==1 && profile->button_count==3 && profile->encoder_count==2);
    } else {
        assert(!strcmp(profile->name,"expanded"));
        assert(layout->buttons==40 && layout->axes==8 && layout->hats==0 && layout->length==21);
        assert(layout->digital_mask==0xffffff && layout->adc_mask==0xff && profile->expander_mask==1);
    }
    osg_profile_state_t state; osg_profile_sample_t sample={0}; uint8_t report[64];
    assert(osg_profile_init(&state,profile,&sample,0));
    osg_profile_update(&state,&sample,0,true,report);
    osg_profile_update(&state,&sample,10,true,report);
    sample.digital_closed=1;
    osg_profile_update(&state,&sample,11,true,report);
    osg_profile_update(&state,&sample,21,true,report);
    assert(report[0]&1);
    osg_profile_rearm(&state,&sample,22);
    osg_profile_update(&state,&sample,50,true,report);
    assert(((report[0]&1)!=0)==profile->buttons[0].maintained);
    assert(state.expansion_faults==profile->expander_mask);
    osg_gateway_profile_t routed; assert(osg_can_profile(profile,layout,&routed));
    assert(routed.button_count==layout->buttons && routed.hat_count==layout->hats && routed.axis_count==layout->axes);
    osg_profile_neutral(&state,report); uint8_t packed[OSG_GW_MAX_REPORT_SIZE]={0};
    memcpy(packed,report,layout->length); assert(osg_gateway_profile_report_valid(&routed,packed));
    uint8_t neutral[OSG_GW_MAX_REPORT_SIZE]; osg_gateway_profile_neutral(&routed,neutral);
    assert(!memcmp(packed,neutral,sizeof packed));
    return 0;
}
