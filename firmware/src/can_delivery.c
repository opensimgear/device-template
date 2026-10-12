#include "can_delivery.h"
#include <string.h>
void osg_can_delivery_rearm(osg_can_delivery_t *d) { memset(d,0xFF,sizeof *d); }
void osg_can_delivery_observe(osg_can_delivery_t *d,const osg_profile_state_t *s,uint32_t next_sequence) {
    for(unsigned i=0;i<s->definition->encoder_count;i++) if(d->phase[i]!=s->encoders[i].phase) {
        d->phase[i]=s->encoders[i].phase; d->minimum[i]=next_sequence;
    }
}
void osg_can_delivery_completed(osg_can_delivery_t *d,osg_profile_state_t *s,uint32_t sequence,
    const uint8_t report[OSG_GW_MAX_REPORT_SIZE],uint32_t now) {
    for(unsigned i=0;i<s->definition->encoder_count;i++) {
        if(d->phase[i]!=s->encoders[i].phase || sequence<d->minimum[i]) continue;
        unsigned bit=s->definition->button_count+2U*i;
        bool plus=(report[bit/8]&(1U<<(bit%8)))!=0; bit++;
        bool minus=(report[bit/8]&(1U<<(bit%8)))!=0;
        if(plus && minus) continue;
        osg_encoder_reported(&s->encoders[i],plus?1:minus?-1:0,now);
    }
}
bool osg_can_profile(const osg_profile_t *p,const osg_profile_layout_t *layout,osg_gateway_profile_t *out) {
    if(!p || !layout || !out || layout->length>OSG_GW_MAX_REPORT_SIZE) return false;
    memset(out,0,sizeof *out); out->button_count=layout->buttons; out->axis_count=layout->axes;
    out->hat_count=layout->hats; out->revision=1;
    for(unsigned i=0;i<p->axis_count;i++) { out->axis_usage[i]=p->axes[i].usage; out->axis_neutral[i]=p->axes[i].neutral; }
    return osg_gateway_profile_valid(out) && osg_gateway_profile_report_size(out)==layout->length;
}
