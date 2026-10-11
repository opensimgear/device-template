#include "tusb.h"
#include "board.h"
#include "stm32g0xx_hal.h"
#include "opensimgear/button.h"
#include "opensimgear/line.h"
#include <stdio.h>
#include <string.h>

static osg_button_t button;
static uint8_t current_report;
static bool led_override, led_state;
static void reply(const char *text) { tud_cdc_write_str(text); tud_cdc_write_flush(); }
static void command(const char *line) {
    if (!strcmp(line, "hello")) reply("Hello from OpenSimGear!\r\n");
    else if (!strcmp(line, "help")) reply("hello | info | led on | led off | led auto | can loopback\r\n");
    else if (!strcmp(line, "info")) {
        char uid[25], out[160]; board_uid_hex(uid);
        snprintf(out, sizeof out, "model=device-template board=starter_g0b1cb uid=%s buttons=1 can=internal-loopback\r\n", uid);
        reply(out);
    } else if (!strcmp(line, "led on")) { led_override = true; led_state = true; reply("OK\r\n"); }
    else if (!strcmp(line, "led off")) { led_override = true; led_state = false; reply("OK\r\n"); }
    else if (!strcmp(line, "led auto")) { led_override = false; reply("OK\r\n"); }
    else if (!strcmp(line, "can loopback")) reply(board_can_loopback() ? "CAN FD LOOPBACK PASS\r\n" : "CAN FD LOOPBACK FAIL\r\n");
    else reply("ERR unknown command\r\n");
}
int main(void) {
    board_init();
    osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10);
    const tusb_rhport_init_t usb = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL};
    if (!tusb_init(0, &usb)) board_fault();
    tud_disconnect(); /* Attach only when VBUS is present, including bus-powered startup. */
    bool attached = false, serial_open = false;
    char line_buffer[64]; osg_line_t line;
    osg_line_init(&line, line_buffer, sizeof line_buffer);
    uint8_t sent = 0; uint32_t last_report = 0;
    for (;;) {
        bool vbus = board_usb_present();
        if (vbus != attached) {
            attached = vbus;
            osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10);
            current_report = 0;
            if (vbus) tud_connect(); else tud_disconnect();
        }
        tud_task();
        bool connected = attached && tud_cdc_connected();
        if (connected != serial_open) {
            serial_open = connected;
            osg_line_init(&line, line_buffer, sizeof line_buffer);
            led_override = false;
            tud_cdc_read_flush();
            if (connected) reply("OpenSimGear hello-world diagnostic console; type help\r\n");
        }
        /* Limit work per loop and only parse when space exists for a whole response. */
        for (unsigned n = 0; connected && n < 16 && tud_cdc_available() && tud_cdc_write_available() >= 160; ++n) {
            char byte; if (tud_cdc_read(&byte, 1) != 1) break;
            osg_line_result_t result = osg_line_feed(&line, byte);
            if (result == OSG_LINE_READY) command(line_buffer);
            else if (result == OSG_LINE_TOO_LONG) reply("ERR line too long\r\n");
        }
        uint32_t now = HAL_GetTick();
        bool pressed = osg_button_update(&button, board_button_pressed(), now);
        current_report = attached && tud_mounted() && !tud_suspended() && pressed ? 1 : 0;
        if (attached && tud_hid_ready() && (current_report != sent || (uint32_t)(now - last_report) >= 10)) {
            if (tud_hid_report(0, &current_report, sizeof current_report)) { sent = current_report; last_report = now; }
        }
        bool blink = ((now / ((attached && tud_mounted()) ? 500U : 1000U)) & 1U) != 0;
        board_led(tud_suspended() ? false : (led_override ? led_state : blink));
    }
}
void tud_mount_cb(void) { current_report = 0; osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10); }
void tud_umount_cb(void) { current_report = 0; osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10); }
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en; current_report = 0; led_override = false;
    osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10);
}
void tud_resume_cb(void) { current_report = 0; osg_button_rearm(&button, board_button_pressed(), HAL_GetTick(), 10); }
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t *buffer, uint16_t length) {
    (void)instance; (void)report_id;
    if (type != HID_REPORT_TYPE_INPUT || length < 1) return 0;
    buffer[0] = current_report; return 1;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, const uint8_t *buffer, uint16_t length) {
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)length;
}
