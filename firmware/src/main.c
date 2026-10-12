#include "tusb.h"
#include "board.h"
#include "profile.h"
#include "hid_delivery.h"
#include "stm32g0xx_hal.h"
#include "opensimgear/line.h"
#include <stdio.h>
#include <string.h>

static osg_profile_state_t inputs;
static uint8_t current_report[OSG_REPORT_CAPACITY];
static uint8_t report_length;
static hid_delivery_t delivery;
static bool led_override, led_state;
static void rearm(void) {
    osg_profile_sample_t sample=board_read_inputs();
    osg_profile_rearm(&inputs,&sample,HAL_GetTick());
    osg_profile_neutral(&inputs,current_report); hid_delivery_invalidate(&delivery);
}
static void reply(const char *text) { tud_cdc_write_str(text); tud_cdc_write_flush(); }
static void command(const char *line) {
    if (!strcmp(line, "hello")) reply("Hello from OpenSimGear!\r\n");
    else if (!strcmp(line, "help")) reply("hello | info | inputs | led on | led off | led auto | can loopback\r\n");
    else if (!strcmp(line, "info")) {
        char uid[25], out[160]; board_uid_hex(uid);
        snprintf(out, sizeof out, "model=device-template board=rev-b profile=%s uid=%s buttons=%u axes=%u hats=%u\r\n",
            device_profile()->name,uid,inputs.layout.buttons,inputs.layout.axes,inputs.layout.hats);
        reply(out);
    } else if (!strcmp(line,"inputs")) {
        char out[160];
        unsigned blocked=0; uint32_t dropped=0,invalid=0;
        for(unsigned i=1;i<256;i++) blocked+=inputs.exclusive_blocked[i];
        for(unsigned i=0;i<device_profile()->encoder_count;i++) {
            dropped+=inputs.encoders[i].dropped; invalid+=inputs.encoders[i].invalid;
        }
        snprintf(out,sizeof out,"axes_ok=%02x hat_conflict=%02x selector_conflict=%02x exp_faults=%02x groups_blocked=%u enc_drop=%lu enc_invalid=%lu\r\n",
            inputs.axis_available,inputs.hat_conflict,inputs.selector_conflict,inputs.expansion_faults,
            blocked,(unsigned long)dropped,(unsigned long)invalid);
        reply(out);
    } else if (!strcmp(line, "led on")) { led_override = true; led_state = true; reply("OK\r\n"); }
    else if (!strcmp(line, "led off")) { led_override = true; led_state = false; reply("OK\r\n"); }
    else if (!strcmp(line, "led auto")) { led_override = false; reply("OK\r\n"); }
    else if (!strcmp(line, "can loopback")) reply(board_can_loopback() ? "CAN FD LOOPBACK PASS\r\n" : "CAN FD LOOPBACK FAIL\r\n");
    else reply("ERR unknown command\r\n");
}
int main(void) {
    if(!device_profile_prepare()) board_fault();
    board_init();
    osg_profile_sample_t initial=board_read_inputs();
    if(!osg_profile_init(&inputs,device_profile(),&initial,HAL_GetTick())) board_fault();
    report_length=inputs.layout.length; osg_profile_neutral(&inputs,current_report);
    const tusb_rhport_init_t usb = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL};
    if (!tusb_init(0, &usb)) board_fault();
    tud_disconnect();
    bool attached = false, serial_open = false, live=false;
    char line_buffer[64]; osg_line_t line;
    osg_line_init(&line, line_buffer, sizeof line_buffer);
    uint8_t sent[OSG_REPORT_CAPACITY]; osg_profile_neutral(&inputs,sent); uint32_t last_report=0,last_sample=HAL_GetTick()-1U;
    for (;;) {
        bool vbus = board_usb_present();
        if (vbus != attached) { attached=vbus; rearm(); if(vbus) tud_connect(); else tud_disconnect(); }
        tud_task();
        bool active=attached && tud_mounted() && !tud_suspended();
        if(active!=live) { live=active; rearm(); }
        uint32_t now=HAL_GetTick();
        if((uint32_t)(now-last_sample)>=1) {
            osg_profile_sample_t sample=board_read_inputs();
            osg_profile_update(&inputs,&sample,now,active,current_report); last_sample=now;
        }
        bool connected = attached && tud_cdc_connected();
        if (connected != serial_open) {
            serial_open = connected; osg_line_init(&line, line_buffer, sizeof line_buffer);
            led_override = false; tud_cdc_read_flush();
            if (connected) reply("OpenSimGear input-example console; type help\r\n");
        }
        for (unsigned n=0; connected && n<16 && tud_cdc_available() && tud_cdc_write_available()>=160; ++n) {
            char byte; if (tud_cdc_read(&byte, 1) != 1) break;
            osg_line_result_t result=osg_line_feed(&line,byte);
            if(result==OSG_LINE_READY) command(line_buffer);
            else if(result==OSG_LINE_TOO_LONG) reply("ERR line too long\r\n");
        }
        if(active && hid_delivery_ready(&delivery,tud_hid_ready()) &&
           (memcmp(current_report,sent,report_length) || (uint32_t)(now-last_report)>=10)) {
            if(tud_hid_report(0,current_report,report_length)) {
                memcpy(sent,current_report,report_length); last_report=now; hid_delivery_submitted(&delivery);
            }
        }
        bool blink=((now/((attached && tud_mounted()) ? 500U : 1000U))&1U)!=0;
        board_led(tud_suspended() ? false : (led_override ? led_state : blink));
    }
}
void tud_mount_cb(void) { delivery.inflight=false; rearm(); }
void tud_umount_cb(void) { delivery.inflight=false; rearm(); }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; led_override=false; rearm(); }
void tud_resume_cb(void) { rearm(); }
void tud_hid_report_complete_cb(uint8_t instance,const uint8_t *report,uint16_t length) {
    if(instance!=0) return;
    bool acknowledge=hid_delivery_completed(&delivery,board_usb_present() && tud_mounted() && !tud_suspended());
    if(length==report_length && acknowledge)
        osg_profile_reported(&inputs,report,HAL_GetTick());
}
void tud_hid_report_failed_cb(uint8_t instance,hid_report_type_t type,const uint8_t *report,uint16_t length) {
    (void)report; (void)length;
    if(instance==0 && type==HID_REPORT_TYPE_INPUT) { delivery.inflight=false; rearm(); }
}
uint16_t tud_hid_get_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t type,uint8_t *buffer,uint16_t length) {
    if(instance!=0 || report_id!=0 || type!=HID_REPORT_TYPE_INPUT || length<report_length) return 0;
    memcpy(buffer,current_report,report_length); return report_length;
}
void tud_hid_set_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t type,const uint8_t *buffer,uint16_t length) {
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)length;
}
