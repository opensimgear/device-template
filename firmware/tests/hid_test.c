#include "tusb.h"
#include "profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void board_uid_hex(char out[25]) { strcpy(out,"0123456789abcdef01234567"); }
static unsigned u16(const uint8_t *p) {return p[0]|p[1]<<8;}
int main(void) {
    assert(device_profile_prepare());
    const osg_profile_layout_t *layout=device_report_layout();
    const uint8_t *config=tud_descriptor_configuration_cb(0);
    unsigned config_length=u16(config+2),report_length=0,interfaces=0;
    for(unsigned i=0;i<config_length;) {
        unsigned n=config[i];assert(n>=2 && i+n<=config_length);
        if(config[i+1]==TUSB_DESC_INTERFACE)interfaces++;
        if(config[i+1]==HID_DESC_TYPE_HID) {assert(n==9);report_length=u16(config+i+7);}
        i+=n;
    }
    assert(interfaces==3 && report_length>0);
    const uint8_t *d=tud_hid_descriptor_report_cb(0);
    unsigned page=0,usage=0,min_usage=0,max_usage=0,size=0,count=0,bits=0,fields=0,collections=0,unit=0;
    unsigned button_fields=0,hat_fields=0,axis_fields=0;
    int32_t logical_min=0,logical_max=0;
    for(unsigned i=0;i<report_length;) {
        uint8_t tag=d[i++];assert(tag!=0xfe);unsigned n=tag&3;n=n==3?4:n;
        assert(i+n<=report_length);uint32_t value=0;
        for(unsigned j=0;j<n;j++)value|=(uint32_t)d[i++]<<(8*j);
        switch(tag&0xfc) {
            case 0x04:page=value;break;
            case 0x08:usage=value;break;
            case 0x18:min_usage=value;break;
            case 0x28:max_usage=value;break;
            case 0x14:logical_min=(n==1)?(int8_t)value:(n==2)?(int16_t)value:(int32_t)value;break;
            case 0x24:logical_max=logical_min<0?((n==1)?(int8_t)value:(n==2)?(int16_t)value:(int32_t)value):(int32_t)value;break;
            case 0x64:unit=value;break;
            case 0x74:size=value;break;
            case 0x94:count=value;break;
            case 0x84:assert(!"Unexpected report ID");break;
            case 0xa0:assert(page==1 && usage==4 && value==1);collections++;usage=0;break;
            case 0xc0:assert(collections==1);collections--;break;
            case 0x80:
                if(!(value&1)) {
                    if(page==9) { button_fields++; assert(size==1 && count==layout->buttons && logical_min==0 && logical_max==1 && min_usage==1 && max_usage==layout->buttons); }
                    else if(usage==0x39) { hat_fields++; assert(page==1 && size==4 && count==layout->hats && logical_min==0 && logical_max==7 && unit==0x14 && value==0x42); }
                    else { axis_fields++; assert(page==1 && usage>=0x30 && usage<=0x38 && size==16 && count==1 && logical_min==0 && logical_max==65535 && unit==0); }
                }
                bits+=size*count;fields++;usage=min_usage=max_usage=0;break;
            case 0x90:case 0xb0:assert(!"Unexpected output/feature field");break;
        }
    }
    assert(bits==8U*layout->length && fields>0 && collections==0);
    assert(button_fields==(layout->buttons?1U:0U) && hat_fields==(layout->hats?1U:0U) && axis_fields==layout->axes);
    const uint16_t *serial=tud_descriptor_string_cb(3,0x409);assert((serial[0]&255)==50 && serial[1]=='0' && serial[24]=='7');
    printf("Profile %s: %u buttons, %u axes, %u hats, %u bytes\n",device_profile()->name,layout->buttons,layout->axes,layout->hats,layout->length);
}
