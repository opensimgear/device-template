#include "tusb.h"
#include "board.h"
#include "profile.h"
#include "hid_delivery.h"
#include "can_client.h"
#include "can_delivery.h"
#include "config_can.h"
#include "stm32g0xx_hal.h"
#include "opensimgear/line.h"
#include <stdio.h>
#include <string.h>

static osg_profile_state_t inputs;
static uint8_t current_report[OSG_REPORT_CAPACITY];
static uint8_t report_length;
static hid_delivery_t delivery;
static bool led_override, led_state;
static osg_can_client_t can_client;
static osg_can_delivery_t can_delivery;
static bool can_initialized,can_reset_requested;
static osg_module_config_can_t config_can;
static char config_response[OSG_RUNTIME_RESPONSE_CAPACITY+64];
static size_t config_response_size,config_response_offset;
static bool config_apply_armed,layout_detached;
static uint32_t config_apply_at,layout_reattach_at;
static bool can_send(void *context,const osg_gateway_frame_t *frame) {
    (void)context; board_can_frame_t tx={.id=frame->id,.size=frame->length,.fd=frame->fd,.brs=frame->brs,
        .deadline_ms=frame->deadline_ms,.deadline_valid=frame->deadline_valid};
    memcpy(tx.data,frame->data,frame->length); return board_can_enqueue(&tx);
}
static void rearm_acquisition(void) {
    osg_profile_sample_t sample=board_read_inputs();
    osg_profile_rearm(&inputs,&sample,HAL_GetTick());
    osg_profile_neutral(&inputs,current_report); hid_delivery_invalidate(&delivery);
    osg_can_delivery_rearm(&can_delivery);
}
static void rearm(void) {
    rearm_acquisition();
    if(can_initialized) { osg_can_client_reset(&can_client); can_reset_requested=true; }
}
static void reply(const char *text) { tud_cdc_write_str(text); tud_cdc_write_flush(); }
static void command(const char *line) {
    if(!strncmp(line,"config ",7)) {
        bool apply=false;osg_runtime_config_t *runtime=device_runtime_config();
        const char *error=osg_runtime_command(runtime,line+7,HAL_GetTick(),&apply);
        int n=error?snprintf(config_response,sizeof config_response,"ERR %s\n",error):snprintf(config_response,sizeof config_response,"OK\n");
        if(n<0 || (size_t)n>=sizeof config_response) board_fault();
        size_t length=osg_runtime_snapshot(runtime,!strcmp(line+7,"get_active"),config_response+n,sizeof config_response-(size_t)n-2);
        if(!length) board_fault();
        config_response_size=(size_t)n+length;memcpy(config_response+config_response_size,"\r\n",2);
        config_response_size+=2;config_response_offset=0;
        if(apply) { config_apply_armed=true;config_apply_at=HAL_GetTick()+100U; }
    }
    else if (!strcmp(line, "hello")) reply("Hello from OpenSimGear!\r\n");
    else if (!strcmp(line, "help")) reply("hello | info | inputs | led on | led off | led auto | can status | config <child-command>\r\n");
    else if (!strcmp(line, "info")) {
        char module[33], out[160]; osg_gateway_identity_format(&can_client.identity,module);
        snprintf(out, sizeof out, "model=device-template board=rev-b profile=%s module=%s protocol=3 buttons=%u axes=%u hats=%u\r\n",
            device_profile()->name,module,inputs.layout.buttons,inputs.layout.axes,inputs.layout.hats);
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
    else if (!strcmp(line, "can status")) {
        board_can_diagnostics_t d=board_can_diagnostics(); char out[160];
        snprintf(out,sizeof out,"can=cyphal node=%u route=%u lease=%lu boot=%lu usb=%u rx=%lu tx=%lu rejected=%lu recoveries=%lu\r\n",
            osg_cyphal_node_id(&can_client.cyphal),can_client.node,(unsigned long)can_client.lease,(unsigned long)can_client.boot,can_client.direct,
            (unsigned long)d.rx,(unsigned long)d.tx,(unsigned long)can_client.rejected,(unsigned long)d.recoveries); reply(out);
    }
    else reply("ERR unknown command\r\n");
}
int main(void) {
    if(!device_profile_prepare()) board_fault();
    board_init();
    if(!device_profile_load(board_settings_io())) board_fault();
    osg_profile_sample_t initial=board_read_inputs();
    if(!osg_profile_init(&inputs,device_profile(),&initial,HAL_GetTick())) board_fault();
    report_length=inputs.layout.length; osg_profile_neutral(&inputs,current_report);
    osg_gateway_identity_t identity; board_module_identity(identity.bytes);
    osg_gateway_profile_t routed_profile;
    if(!osg_can_profile(device_profile(),&inputs.layout,&routed_profile)) board_fault();
    if(!osg_can_client_init(&can_client,&identity,&routed_profile,board_boot_session(),can_send,NULL)) board_fault();
    can_client.configuration_generation=device_runtime_config()->generation;
    can_initialized=true; osg_can_delivery_rearm(&can_delivery);
    osg_module_config_can_init(&config_can,&can_client,device_runtime_config());
    const tusb_rhport_init_t usb = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL};
    if (!tusb_init(0, &usb)) board_fault();
    tud_disconnect();
    bool attached = false, serial_open = false, live=false, direct_live=false;
    uint32_t can_generation=can_client.generation; bool was_recovering=false;
    char line_buffer[OSG_RUNTIME_COMMAND_CAPACITY+8]; osg_line_t line;
    osg_line_init(&line, line_buffer, sizeof line_buffer);
    uint8_t sent[OSG_REPORT_CAPACITY]; osg_profile_neutral(&inputs,sent); uint32_t last_report=0,last_sample=HAL_GetTick()-1U;
    for (;;) {
        uint32_t before=HAL_GetTick();
        if(layout_detached && (int32_t)(before-layout_reattach_at)>=0) layout_detached=false;
        bool vbus = board_usb_present() && !layout_detached;
        if (vbus != attached) {
            attached=vbus;rearm();
            if(!vbus && !layout_detached) osg_runtime_invalidate_claim(device_runtime_config());
            if(vbus) tud_connect(); else tud_disconnect();
        }
        tud_task();
        uint32_t now=HAL_GetTick();
        osg_runtime_tick(device_runtime_config(),now);
        bool direct=attached && tud_mounted() && !tud_suspended();
        if(direct!=direct_live) {
            direct_live=direct; rearm(); (void)osg_can_client_usb(&can_client,direct);
        }
        if(can_reset_requested) { board_can_reset(now); can_reset_requested=false; }
        board_can_task(now); board_can_diagnostics_t diag=board_can_diagnostics();
        if(diag.recovering && !was_recovering) rearm();
        was_recovering=diag.recovering;
        bool active=direct || osg_can_client_active(&can_client);
        if(active!=live || can_generation!=can_client.generation) {
            /* Rearm acquisition without discarding a newly received lease. */
            rearm_acquisition(); live=active; can_generation=can_client.generation;
        }
        if((uint32_t)(now-last_sample)>=1) {
            osg_profile_sample_t sample=board_read_inputs();
            osg_profile_update(&inputs,&sample,now,active,current_report); last_sample=now;
            osg_runtime_transform_report(device_runtime_config(),&inputs,current_report);
        }
        osg_can_delivery_observe(&can_delivery,&inputs,can_client.next_sequence);
        for(unsigned n=0;n<4;n++) {
            board_can_frame_t rx; if(!board_can_receive(&rx)) break;
            osg_gateway_frame_t frame={.id=rx.id,.length=rx.size,.extended=true,.fd=rx.fd,.brs=rx.brs};
            memcpy(frame.data,rx.data,rx.size);
            uint8_t source=osg_cyphal_node_id(&can_client.cyphal);
            if(osg_can_client_receive(&can_client,&frame,now))
                osg_can_delivery_completed(&can_delivery,&inputs,can_client.ack_sequence,can_client.acknowledged,now);
            if(source!=osg_cyphal_node_id(&can_client.cyphal)) {
                /* Frames already handed to the controller still carry the
                 * old source ID. Purge them along with the invalidated route. */
                board_can_reset(now); break;
            }
        }
        if(can_generation!=can_client.generation) {
            /* A lease arriving after sampling must not publish an old pulse. */
            rearm_acquisition(); can_generation=can_client.generation;
            live=direct || osg_can_client_active(&can_client);
        }
        uint8_t routed[OSG_GW_MAX_REPORT_SIZE]={0}; memcpy(routed,current_report,report_length);
        osg_can_client_tick(&can_client,now,device_runtime_config()->pending_apply?NULL:routed);
        osg_module_config_can_tick(&config_can,now);
        if(device_runtime_config()->pending_apply && !config_apply_armed) {
            config_apply_armed=true;config_apply_at=now+100U;
        }
        bool connected = attached && tud_cdc_connected();
        if (connected != serial_open) {
            serial_open = connected; osg_line_init(&line, line_buffer, sizeof line_buffer);
            led_override = false;
            if(!connected) {
                /* DTR and the first command may arrive in one USB task. */
                tud_cdc_read_flush();
                config_response_offset=config_response_size=0;
                if(!layout_detached) osg_runtime_invalidate_claim(device_runtime_config());
            }
            if (connected) reply("OpenSimGear generic Cyphal device console; type help\r\n");
        }
        if(connected && config_response_offset<config_response_size) {
            config_response_offset+=tud_cdc_write(config_response+config_response_offset,(uint32_t)(config_response_size-config_response_offset));
            tud_cdc_write_flush();
            if(config_response_offset==config_response_size) config_response_offset=config_response_size=0;
        }
        for (unsigned n=0; connected && !config_response_size && !device_runtime_config()->pending_apply && n<16 && tud_cdc_available() && tud_cdc_write_available()>=160; ++n) {
            char byte; if (tud_cdc_read(&byte, 1) != 1) break;
            osg_line_result_t result=osg_line_feed(&line,byte);
            if(result==OSG_LINE_READY) command(line_buffer);
            else if(result==OSG_LINE_TOO_LONG) reply("ERR line too long\r\n");
        }
        if(config_apply_armed && config_response_size && (int32_t)(now-config_apply_at)>=3900) {
            /* An unread acknowledgement cannot hold an accepted Apply forever.
             * The host can reconnect and read the active generation back. */
            config_response_offset=config_response_size=0;tud_cdc_write_clear();
        }
        if(config_apply_armed && !config_response_size && !osg_module_config_can_busy(&config_can) && !board_can_diagnostics().tx_queue && (int32_t)(now-config_apply_at)>=0) {
            /* Publish a changed descriptor only while physically detached.
             * Old USB/CAN delivery generations and controller queues are purged. */
            tud_disconnect();attached=false;layout_detached=true;layout_reattach_at=now+250U;
            direct_live=false;live=false;delivery.inflight=false;rearm();board_can_reset(now);can_reset_requested=false;
            if(!osg_runtime_finish_apply(device_runtime_config())) board_fault();
            board_inputs_reconfigured();osg_profile_sample_t sample=board_read_inputs();
            if(!osg_profile_init(&inputs,device_profile(),&sample,now)) board_fault();
            report_length=inputs.layout.length;osg_profile_neutral(&inputs,current_report);osg_profile_neutral(&inputs,sent);
            if(!osg_can_profile(device_profile(),&inputs.layout,&can_client.profile)) board_fault();
            can_client.configuration_generation=device_runtime_config()->generation;
            osg_can_client_reset(&can_client);(void)osg_can_client_usb(&can_client,false);
            osg_can_delivery_rearm(&can_delivery);can_generation=can_client.generation;last_report=now;last_sample=now-1U;
            config_apply_armed=false;
        }
        if(direct && !layout_detached && !device_runtime_config()->pending_apply && hid_delivery_ready(&delivery,tud_hid_ready()) &&
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
void tud_umount_cb(void) { delivery.inflight=false; rearm();if(!layout_detached) osg_runtime_invalidate_claim(device_runtime_config()); }
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
