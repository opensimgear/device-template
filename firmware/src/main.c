#include "tusb.h"
#include "board.h"
#include "profile.h"
#include "stm32g0xx_hal.h"
#include "opensimgear/line.h"
#include <stdio.h>
#include <string.h>

static profile_t inputs;
static uint8_t current_report[4]={0,8,0,128};
static uint32_t generation, pending_generation;
static bool led_override, led_state, inflight;
static void rearm(void) {
    profile_sample_t sample=board_read_inputs();
    profile_rearm(&inputs,&sample,HAL_GetTick());
    profile_neutral(current_report); generation++;
}
static void reply(const char *text) { tud_cdc_write_str(text); tud_cdc_write_flush(); }
static void command(const char *line) {
    if (!strcmp(line, "hello")) reply("Hello from OpenSimGear!\r\n");
    else if (!strcmp(line, "help")) reply("hello | info | inputs | led on | led off | led auto | can loopback\r\n");
    else if (!strcmp(line, "info")) {
        char uid[25], out[160]; board_uid_hex(uid);
        snprintf(out, sizeof out, "model=device-template board=starter_g0b1cb uid=%s buttons=8 axes=1 hats=1 can=internal-loopback\r\n", uid);
        reply(out);
    } else if (!strcmp(line,"inputs")) {
        char out[160];
        snprintf(out,sizeof out,"buttons=%u x=%u hat=%u axis_ok=%u hat_conflict=%u selector_conflict=%u enc_drop=%lu enc_invalid=%lu\r\n",
            (unsigned)current_report[0],(unsigned)(current_report[2]|current_report[3]<<8),(unsigned)current_report[1],
            (unsigned)inputs.axis_available,(unsigned)inputs.hat_conflict,(unsigned)inputs.selector_conflict,
            (unsigned long)inputs.encoder.dropped,(unsigned long)inputs.encoder.invalid);
        reply(out);
    } else if (!strcmp(line, "led on")) { led_override = true; led_state = true; reply("OK\r\n"); }
    else if (!strcmp(line, "led off")) { led_override = true; led_state = false; reply("OK\r\n"); }
    else if (!strcmp(line, "led auto")) { led_override = false; reply("OK\r\n"); }
    else if (!strcmp(line, "can loopback")) reply(board_can_loopback() ? "CAN FD LOOPBACK PASS\r\n" : "CAN FD LOOPBACK FAIL\r\n");
    else reply("ERR unknown command\r\n");
}
int main(void) {
    board_init();
    profile_sample_t initial=board_read_inputs(); profile_init(&inputs,&initial,HAL_GetTick());
    const tusb_rhport_init_t usb = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL};
    if (!tusb_init(0, &usb)) board_fault();
    tud_disconnect();
    bool attached = false, serial_open = false, live=false;
    char line_buffer[64]; osg_line_t line;
    osg_line_init(&line, line_buffer, sizeof line_buffer);
    uint8_t sent[4]={0,8,0,128}; uint32_t last_report=0,last_sample=HAL_GetTick()-1U;
    for (;;) {
        bool vbus = board_usb_present();
        if (vbus != attached) { attached=vbus; rearm(); if(vbus) tud_connect(); else tud_disconnect(); }
        tud_task();
        bool active=attached && tud_mounted() && !tud_suspended();
        if(active!=live) { live=active; rearm(); }
        uint32_t now=HAL_GetTick();
        if((uint32_t)(now-last_sample)>=1) {
            profile_sample_t sample=board_read_inputs();
            profile_update(&inputs,&sample,now,active,current_report); last_sample=now;
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
        if(active && !inflight && tud_hid_ready() &&
           (memcmp(current_report,sent,sizeof sent) || (uint32_t)(now-last_report)>=10)) {
            if(tud_hid_report(0,current_report,sizeof current_report)) {
                memcpy(sent,current_report,sizeof sent); last_report=now; pending_generation=generation; inflight=true;
            }
        }
        bool blink=((now/((attached && tud_mounted()) ? 500U : 1000U))&1U)!=0;
        board_led(tud_suspended() ? false : (led_override ? led_state : blink));
    }
}
void tud_mount_cb(void) { inflight=false; rearm(); }
void tud_umount_cb(void) { inflight=false; rearm(); }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; led_override=false; rearm(); }
void tud_resume_cb(void) { rearm(); }
void tud_hid_report_complete_cb(uint8_t instance,const uint8_t *report,uint16_t length) {
    if(instance!=0) return;
    inflight=false;
    if(length==sizeof current_report && pending_generation==generation && board_usb_present() && tud_mounted() && !tud_suspended())
        profile_reported(&inputs,report,HAL_GetTick());
}
void tud_hid_report_failed_cb(uint8_t instance,hid_report_type_t type,const uint8_t *report,uint16_t length) {
    (void)report; (void)length;
    if(instance==0 && type==HID_REPORT_TYPE_INPUT) { inflight=false; rearm(); }
}
uint16_t tud_hid_get_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t type,uint8_t *buffer,uint16_t length) {
    if(instance!=0 || report_id!=0 || type!=HID_REPORT_TYPE_INPUT || length<sizeof current_report) return 0;
    memcpy(buffer,current_report,sizeof current_report); return sizeof current_report;
}
void tud_hid_set_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t type,const uint8_t *buffer,uint16_t length) {
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)length;
}
