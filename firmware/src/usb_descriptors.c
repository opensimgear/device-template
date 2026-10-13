#include "tusb.h"
#include "opensimgear/usb_descriptor.h"
#include "board.h"
#include "profile.h"
#include <string.h>

const uint8_t *tud_descriptor_device_cb(void) {
    static tusb_desc_device_t descriptor;
    descriptor = osg_usb_device_descriptor(OSG_USB_VID, OSG_USB_PID, 0x0003);
    return (const uint8_t *)&descriptor;
}
const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    if(index) return NULL;
    static uint8_t descriptor[TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN];
    size_t report_length; device_report_descriptor(&report_length);
    osg_usb_hid_cdc_configuration(descriptor, sizeof descriptor, (uint16_t)report_length);
    return descriptor;
}
const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
    if(instance!=0) return NULL;
    size_t length; return device_report_descriptor(&length);
}
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid; static uint16_t descriptor[64]; static char uid[25];
    const char *text = NULL;
    if (index == 0) { descriptor[0] = (TUSB_DESC_STRING << 8) | 4; descriptor[1] = 0x0409; return descriptor; }
    switch (index) {
        case 1: text = "OpenSimGear"; break;
        case 2: text = "Device Template v0.1.0"; break;
        case 3: board_uid_hex(uid); text = uid; break;
        case 4: text = "Diagnostic Serial"; break;
        case 5: text = device_profile()->name; break;
        default: return NULL;
    }
    size_t length = osg_runtime_utf16(text,descriptor+1,63);
    if(!length) return NULL;
    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * length + 2));
    return descriptor;
}
