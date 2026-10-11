#include "tusb.h"
#include "opensimgear/usb_descriptor.h"
#include "board.h"
#include <string.h>

/* Device profile: one joystick button, seven padding bits; no synthetic axes. */
static const uint8_t report[] = {0x05,0x01,0x09,0x04,0xa1,0x01,
    0x05,0x09,0x19,0x01,0x29,0x01,0x15,0x00,0x25,0x01,
    0x75,0x01,0x95,0x01,0x81,0x02,0x75,0x07,0x95,0x01,0x81,0x03,0xc0};
const uint8_t *tud_descriptor_device_cb(void) {
    static tusb_desc_device_t descriptor;
    descriptor = osg_usb_device_descriptor(OSG_USB_VID, OSG_USB_PID, 0x0001);
    return (const uint8_t *)&descriptor;
}
const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index; static uint8_t descriptor[TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN];
    osg_usb_hid_cdc_configuration(descriptor, sizeof descriptor, sizeof report);
    return descriptor;
}
const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) { (void)instance; return report; }
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid; static uint16_t descriptor[64]; static char uid[25];
    const char *text = NULL;
    if (index == 0) { descriptor[0] = (TUSB_DESC_STRING << 8) | 4; descriptor[1] = 0x0409; return descriptor; }
    switch (index) {
        case 1: text = "OpenSimGear"; break;
        case 2: text = "Device Template Hello World"; break;
        case 3: board_uid_hex(uid); text = uid; break;
        case 4: text = "Diagnostic Serial"; break;
        case 5: text = "Test Button"; break;
        default: return NULL;
    }
    size_t length = strlen(text); if (length > 63) length = 63;
    for (size_t i = 0; i < length; ++i) descriptor[i + 1] = (uint8_t)text[i];
    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * length + 2));
    return descriptor;
}
