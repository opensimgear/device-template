#include "tusb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void board_uid_hex(char out[25]) { strcpy(out,"0123456789abcdef01234567"); }
static unsigned u16(const uint8_t *p) {return p[0]|p[1]<<8;}
int main(void) {
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
                if(fields==0)assert(bits==0 && page==9 && min_usage==1 && max_usage==8 && size==1 && count==8 && value==2 && logical_min==0 && logical_max==1);
                else if(fields==1)assert(bits==8 && page==1 && usage==0x39 && size==4 && count==1 && value==0x42 && logical_min==0 && logical_max==7 && unit==0x14);
                else if(fields==2)assert(bits==12 && size==4 && count==1 && (value&1));
                else if(fields==3)assert(bits==16 && page==1 && usage==0x30 && size==16 && count==1 && value==2 && logical_min==0 && logical_max==65535 && unit==0);
                else assert(!"Unexpected input field");
                bits+=size*count;fields++;usage=min_usage=max_usage=0;break;
            case 0x90:case 0xb0:assert(!"Unexpected output/feature field");break;
        }
    }
    assert(bits==32 && fields==4 && collections==0);
    const uint16_t *serial=tud_descriptor_string_cb(3,0x409);assert((serial[0]&255)==50 && serial[1]=='0' && serial[24]=='7');
    puts("Actual HID report exposes eight buttons, an eight-way null-state hat and unsigned X in four bytes");
}
