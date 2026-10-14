#include "device_actions.h"
#include "config_can.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char reply[OSG_ACTION_REPLY_CAPACITY];
static void command(osg_device_actions_t *s,const char *line,bool ok) {
    assert(osg_actions_command(&s->registry,line,reply,sizeof reply));
    assert(strstr(reply,ok ? "\"ok\":true" : "\"ok\":false"));
}
static void hardware_test(void) {
    osg_device_actions_t s;
    assert(!osg_device_actions_init(NULL,false));
    assert(osg_device_actions_init(&s,false));
    command(&s,"action describe 0",true);
    assert(strstr(reply,"led.status.set_mode") && !strstr(reply,"led.rgb"));
    assert(osg_device_actions_status(&s,true,false));
    assert(!osg_device_actions_status(&s,false,false));
    assert(!osg_device_actions_status(NULL,true,false));
    command(&s,"action invoke 1 led.status.set_mode on",true);
    assert(s.status_mode==OSG_LED_ON && osg_device_actions_status(&s,false,false));
    assert(!osg_device_actions_status(&s,true,true));
    command(&s,"action state",true); assert(strstr(reply,"\"on\""));
    command(&s,"action invoke 1 led.status.set_mode off",true);
    assert(s.status_mode==OSG_LED_OFF && !osg_device_actions_status(&s,true,false));
    command(&s,"action invoke 2 led.status.set_mode auto",false); assert(s.status_mode==OSG_LED_OFF);
    command(&s,"action invoke 1 led.status.set_mode invalid",false); assert(s.status_mode==OSG_LED_OFF);
    command(&s,"action invoke 1 led.rgb.set_color ff0000",false);
    command(&s,"action invoke 1 led.status.set_mode auto",true); assert(s.status_mode==OSG_LED_AUTO);
    assert(osg_device_actions_init(&s,false) && s.status_mode==OSG_LED_AUTO);
}
static void simulation_test(void) {
    osg_device_actions_t s; assert(osg_device_actions_init(&s,true));
    command(&s,"action describe 0",true);
    assert(strstr(reply,"led.rgb.set_color") && strstr(reply,"led.boolean.set_enabled") && !strstr(reply,"led.status"));
    command(&s,"action invoke 1 led.rgb.set_color 12abef",true); assert(s.rgb==0x12abef);
    command(&s,"action invoke 1 led.boolean.set_enabled 1",true); assert(s.enabled);
    command(&s,"action state",true); assert(strstr(reply,"12ABEF") && strstr(reply,"true"));
    command(&s,"action invoke 1 led.rgb.set_color 1234567",false); assert(s.rgb==0x12abef);
    command(&s,"action invoke 1 led.boolean.set_enabled 2",false); assert(s.enabled);
    command(&s,"action invoke 1 led.boolean.set_enabled 0 extra",false); assert(s.enabled);
    command(&s,"action invoke 1 led.boolean.set_enabled 0",true); assert(!s.enabled);
    assert(osg_device_actions_init(&s,true) && !s.rgb && !s.enabled);
}
typedef struct {char text[64];unsigned writes;bool write_fail,read_fail;} display_t;
static bool write_display(void *context,const char *text) {
    display_t *s=context;if(s->write_fail) return false;
    strcpy(s->text,text);s->writes++;return true;
}
static bool read_display(void *context,char *text,size_t capacity) {
    display_t *s=context;if(s->read_fail || strlen(s->text)>=capacity) return false;
    strcpy(text,s->text);return true;
}
static void display_test(void) {
    display_t frequency={0},text={0};
    osg_device_display_port_t ports[]={
        {.id="display.frequency.set_text",.label="Frequency",.endpoint="display.frequency",
         .display={7,1,OSG_ACTION_DISPLAY_ASCII},.max_bytes=7,.write=write_display,.read=read_display,.context=&frequency},
        {.id="display.text.set_text",.label="Text",.endpoint="display.text",
         .display={8,2,OSG_ACTION_DISPLAY_UTF8},.max_bytes=63,.write=write_display,.read=read_display,.context=&text}
    };
    osg_device_actions_t actions;
    assert(!osg_device_actions_init_outputs(NULL,false,ports,2));
    assert(!osg_device_actions_init_outputs(&actions,false,NULL,1));
    assert(!osg_device_actions_init_outputs(&actions,false,ports,OSG_DEVICE_DISPLAY_MAX+1));
    osg_device_display_port_t invalid=ports[0];invalid.write=NULL;
    assert(!osg_device_actions_init_outputs(&actions,false,&invalid,1));
    invalid=ports[0];invalid.read=NULL;assert(!osg_device_actions_init_outputs(&actions,false,&invalid,1));
    invalid=ports[0];invalid.display.columns=0;assert(!osg_device_actions_init_outputs(&actions,false,&invalid,1));
    const char *reserved[]={"led.status.set_mode","led.rgb.set_color","led.boolean.set_enabled"};
    for(unsigned i=0;i<sizeof reserved/sizeof reserved[0];i++) for(unsigned simulated=0;simulated<2;simulated++) {
        invalid=ports[0];invalid.id=reserved[i];
        assert(!osg_device_actions_init_outputs(&actions,simulated!=0,&invalid,1));
        assert(frequency.writes==0); /* Reject before a string can alias LED dispatch. */
    }
    assert(osg_device_actions_init_outputs(&actions,false,ports,2));
    command(&actions,"action describe 0",true);
    assert(strstr(reply,"\"display\":{\"columns\":7,\"rows\":1,\"encoding\":\"ascii\"}"));
    assert(strstr(reply,"\"columns\":8,\"rows\":2,\"encoding\":\"utf8\""));
    command(&actions,"action invoke 1 display.frequency.set_text 3235312e303030",true);
    assert(!strcmp(frequency.text,"251.000") && frequency.writes==1);
    command(&actions,"action invoke 1 display.frequency.set_text 3235312e30303030",false);
    command(&actions,"action invoke 1 display.frequency.set_text c3a9",false);
    assert(frequency.writes==1 && !strcmp(frequency.text,"251.000"));
    command(&actions,"action invoke 1 display.text.set_text c3a9e6b3a2",true);
    assert(!strcmp(text.text,"\xc3\xa9\xe6\xb3\xa2") && text.writes==1);
    command(&actions,"action invoke 1 display.text.set_text 3031323334353637383961626364656630",false);
    assert(text.writes==1);
    frequency.write_fail=true;command(&actions,"action invoke 1 display.frequency.set_text 313233",false);
    assert(frequency.writes==1);frequency.write_fail=false;
    frequency.read_fail=true;command(&actions,"action invoke 1 display.frequency.set_text 313233",false);
    assert(!strcmp(frequency.text,"123") && frequency.writes==2);frequency.read_fail=false;
    command(&actions,"action state",true);assert(strstr(reply,"123"));
    strcpy(frequency.text,"\xc3\xa9");command(&actions,"action state",false);
    command(&actions,"action invoke 1 display.frequency.set_text -",true);assert(!frequency.text[0]);
}
static void transform_axes(void *context,const osg_profile_state_t *state,uint8_t report[OSG_REPORT_CAPACITY]) {
    osg_runtime_transform_report(context,state,report);
}
static void calibrated_event_test(void) {
    osg_runtime_definition_t defaults={.name="Axis panel",.axis_count=1};
    defaults.axes[0]=(osg_profile_axis_t){0,0x30,100,3900,32768};
    defaults.axis_options[0]=(osg_runtime_axis_t){true,1000};
    osg_runtime_config_t runtime;assert(osg_runtime_init_definition(&runtime,&defaults,NULL));
    osg_profile_sample_t sample={.axes_fresh=1};sample.axes[0]=100;
    osg_events_t events;assert(osg_events_init(&events,&runtime.profile,&sample,0,runtime.generation));
    osg_events_set_transform(&events,transform_axes,&runtime);osg_events_observe(&events,&sample,10);
    assert(osg_events_command(&events,"events state",reply,sizeof reply));
    assert(strstr(reply,"\"axes\":[65535]") && strstr(reply,"\"axisAvailable\":[true]"));
    sample.axes[0]=2000;osg_events_observe(&events,&sample,20);
    assert(osg_events_command(&events,"events state",reply,sizeof reply));assert(strstr(reply,"\"axes\":[32768]"));
    sample.axes_fresh=0;osg_events_observe(&events,&sample,121);
    assert(osg_events_command(&events,"events state",reply,sizeof reply));
    assert(strstr(reply,"\"axisAvailable\":[false]") && strstr(reply,"\"axisNeutral\":[32768]"));
}
static uint32_t request_now=1;
static bool can_command(osg_module_config_can_t *s,uint32_t sequence,const char *command) {
    size_t length=strlen(command);
    for(size_t offset=0;offset<length;offset+=OSG_GW_CONFIG_CHUNK_SIZE) {
        osg_gateway_message_t m={.type=OSG_GW_CONFIG_REQUEST,.identity=s->client->identity,
            .node=s->client->node,.source_node=s->client->gateway_source,.boot_session=s->client->boot,
            .lease=s->client->lease,.gateway_epoch=s->client->epoch,.sequence=sequence,
            .config_total=(uint16_t)length,.config_offset=(uint16_t)offset};
        size_t remaining=length-offset;
        m.config_length=(uint8_t)(remaining>OSG_GW_CONFIG_CHUNK_SIZE ? OSG_GW_CONFIG_CHUNK_SIZE : remaining);
        memcpy(m.config_data,command+offset,m.config_length);
        if(!osg_module_config_can_receive(s,&m,request_now)) return false;
    }
    return true;
}
static unsigned transmitted;
typedef struct {osg_can_client_t *client;uint32_t now;} route_peer_t;
static bool route_peer_send(void *context,const osg_gateway_frame_t *frame) {
    route_peer_t *peer=context;(void)osg_can_client_receive(peer->client,frame,peer->now);return true;
}
static bool send_frame(void *context,const osg_gateway_frame_t *frame) {
    (void)context; assert(frame->extended && frame->length); transmitted++; return true;
}
static void flush_response(osg_module_config_can_t *s) {
    assert(osg_module_config_can_busy(s));
    for(unsigned i=0;s->responding && i<256;i++) osg_module_config_can_tick(s,request_now+1+i);
    assert(!s->responding && !osg_module_config_can_busy(s) && transmitted);
}
static void can_action_test(void) {
    osg_gateway_identity_t identity={{'O','S','G','D',1}};
    osg_gateway_profile_t profile={.button_count=1,.revision=1};
    osg_can_client_t client;
    assert(osg_can_client_init(&client,&identity,&profile,7,send_frame,NULL));
    assert(canard_set_node_id(&client.cyphal.canard,21));
    client.node=3;client.gateway_source=100;client.lease=10;client.epoch=2;client.direct=true;
    osg_runtime_config_t runtime={0}; osg_device_actions_t actions;
    assert(osg_device_actions_init(&actions,true));
    osg_module_config_can_t can; osg_module_config_can_init(&can,&client,&runtime);
    osg_module_config_can_actions(&can,&actions.registry);
    assert(can_command(&can,1,"action invoke 1 led.rgb.set_color ff0000"));
    assert(actions.rgb==0xff0000 && !runtime.pending_apply && !runtime.generation);
    assert(can.cached && can.responding && !strncmp(can.response,"OK\n",3));
    assert(strstr(can.response,"\"ok\":true"));
    flush_response(&can);
    command(&actions,"action invoke 1 led.rgb.set_color 0000ff",true);
    assert(can_command(&can,1,"action invoke 1 led.rgb.set_color ff0000"));
    assert(actions.rgb==0x0000ff); /* Same request replays its immutable reply. */
    assert(!can_command(&can,1,"action invoke 1 led.rgb.set_color 00ff00"));
    flush_response(&can);
    assert(can_command(&can,2,"action invoke 1 led.boolean.set_enabled 1"));
    assert(actions.enabled && !runtime.pending_apply && !runtime.generation);
    flush_response(&can);
    assert(can_command(&can,3,"action invoke 2 led.boolean.set_enabled 0"));
    assert(strstr(can.response,"\"ok\":false") && actions.enabled);
    client.lease=11; /* Fresh lease invalidates the old cached route. */
    osg_module_config_can_tick(&can,300); assert(!can.cached && !can.responding);
    assert(can_command(&can,4,"action state"));
    assert(strstr(can.response,"\"ok\":true") && actions.enabled);
    flush_response(&can);
    assert(can_command(&can,5,"get_active"));
    assert(!strncmp(can.response,"OK\n",3) && strstr(can.response,"\"generation\""));
    flush_response(&can);
    assert(can_command(&can,6,"unknown"));
    assert(!strncmp(can.response,"ERR invalid_request\n",20));
    flush_response(&can);
    /* An incomplete transfer expires without executing a setter. */
    const char *incomplete="action invoke 1 led.rgb.set_color 00ff00";
    osg_gateway_message_t partial={.type=OSG_GW_CONFIG_REQUEST,.identity=identity,.node=3,.source_node=100,
        .boot_session=7,.lease=11,.gateway_epoch=2,.sequence=7,.config_total=(uint16_t)strlen(incomplete),
        .config_length=OSG_GW_CONFIG_CHUNK_SIZE};
    memcpy(partial.config_data,incomplete,partial.config_length);
    assert(osg_module_config_can_receive(&can,&partial,400));
    assert(can.assembling && osg_module_config_can_busy(&can));
    osg_module_config_can_tick(&can,4400); assert(!can.assembling && actions.rgb==0x0000ff);
    assert(!osg_module_config_can_receive(&can,NULL,4400));
    partial.sequence=0; assert(!osg_module_config_can_receive(&can,&partial,4400));
    osg_module_config_can_tick(NULL,4400); assert(!osg_module_config_can_busy(NULL));
    osg_cyphal_destroy(&client.cyphal);
}

static void lease_test(void) {
    osg_device_actions_t s;assert(osg_device_actions_init(&s,false));
    const char *secret="00112233-4455-6677-8899-aabbccddeeff";
    char line[256];snprintf(line,sizeof line,"action claim 1 led.status.set_mode %s 500 off",secret);
    assert(osg_actions_command_at(&s.registry,line,10,OSG_ACTION_SCOPE_CAN,reply,sizeof reply));
    assert(strstr(reply,"\"leaseId\":1") && s.status_mode==OSG_LED_AUTO);
    assert(!osg_device_actions_legacy_led(&s,"on",11));
    snprintf(line,sizeof line,"action invoke_owned %s 1 1 led.status.set_mode on",secret);
    assert(osg_actions_command_at(&s.registry,line,12,OSG_ACTION_SCOPE_CAN,reply,sizeof reply));
    assert(strstr(reply,"\"ok\":true") && s.status_mode==OSG_LED_ON);
    osg_actions_invalidate_scope(&s.registry,OSG_ACTION_SCOPE_USB,13);assert(s.status_mode==OSG_LED_ON);
    osg_actions_tick(&s.registry,510);assert(s.status_mode==OSG_LED_OFF);
    assert(osg_device_actions_legacy_led(&s,"auto",511));assert(s.status_mode==OSG_LED_AUTO);
    assert(!osg_device_actions_legacy_led(NULL,"on",511) && !osg_device_actions_legacy_led(&s,NULL,511));
}
static void can_lease_test(void) {
    osg_gateway_identity_t identity={{'O','S','G','L',1}};
    osg_gateway_profile_t profile={.revision=1};osg_can_client_t client;
    assert(osg_can_client_init(&client,&identity,&profile,9,send_frame,NULL));
    assert(canard_set_node_id(&client.cyphal.canard,23));
    client.node=5;client.gateway_source=100;client.lease=30;client.epoch=2;
    osg_runtime_config_t runtime={0};osg_device_actions_t actions;assert(osg_device_actions_init(&actions,true));
    osg_module_config_can_t can;osg_module_config_can_init(&can,&client,&runtime);osg_module_config_can_actions(&can,&actions.registry);
    request_now=10;
    const char *claim="action claim 1 led.rgb.set_color 00112233-4455-6677-8899-aabbccddeeff 1000 000000";
    assert(can_command(&can,1,claim));assert(actions.registry.leases[0].expires_at==1010);flush_response(&can);
    request_now=500;assert(can_command(&can,1,claim)); /* Cached retry cannot renew. */
    assert(actions.registry.leases[0].expires_at==1010);flush_response(&can);
    assert(can_command(&can,2,"action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 1 1 led.rgb.set_color 12abef"));
    assert(actions.rgb==0x12abef);flush_response(&can);
    osg_module_config_can_tick(&can,1010);assert(!actions.rgb && !actions.registry.leases[0].owned);
    request_now=1100;assert(can_command(&can,3,claim));flush_response(&can);
    assert(can_command(&can,4,"action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 2 1 led.rgb.set_color ffffff"));
    assert(actions.rgb==0xffffff);client.lease++;osg_module_config_can_tick(&can,1101);
    assert(!actions.rgb && !actions.registry.leases[0].owned && !can.cached && !can.responding);
    request_now=1200;
    assert(can_command(&can,5,"action claim 1 led.rgb.set_color 00112233-4455-6677-8899-aabbccddeeff 5000 000000"));flush_response(&can);
    assert(can_command(&can,6,"action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 3 1 led.rgb.set_color ffffff"));flush_response(&can);
    const char *incomplete="action state";
    osg_gateway_message_t partial={.type=OSG_GW_CONFIG_REQUEST,.identity=identity,.node=client.node,.source_node=client.gateway_source,
        .boot_session=client.boot,.lease=client.lease,.gateway_epoch=client.epoch,.sequence=7,.config_total=(uint16_t)strlen(incomplete),.config_length=6};
    memcpy(partial.config_data,incomplete,partial.config_length);
    assert(osg_module_config_can_receive(&can,&partial,1300));
    osg_module_config_can_tick(&can,5300);assert(!can.assembling && !can.cached && !can.responding);
    client.lease++;osg_module_config_can_tick(&can,5301);
    assert(!actions.rgb && !actions.registry.leases[0].owned);
    /* A fresh scope can acquire again; offline loss is also immediate. */
    request_now=5400;
    assert(can_command(&can,8,"action claim 1 led.rgb.set_color 00112233-4455-6677-8899-aabbccddeeff 5000 000000"));flush_response(&can);
    assert(can_command(&can,9,"action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 4 1 led.rgb.set_color ffffff"));flush_response(&can);
    uint32_t route_generation=client.route_generation;
    assert(osg_can_client_usb(&client,true));osg_module_config_can_tick(&can,5401);
    assert(actions.rgb==0xffffff && actions.registry.leases[0].owned && client.route_generation==route_generation);
    assert(osg_can_client_usb(&client,false));
    client.assigned_at=3400;
    route_peer_t peer={&client,5500};osg_cyphal_t gateway;
    osg_gateway_identity_t gateway_id={{'G','W',1}};
    assert(osg_cyphal_init(&gateway,&gateway_id,1,"org.opensimgear.gateway",route_peer_send,&peer));
    assert(canard_set_node_id(&gateway.canard,client.gateway_source));
    osg_gateway_message_t assignment={.type=OSG_GW_ASSIGN,.identity=identity,.node=client.node,.boot_session=client.boot,.lease=client.lease,.gateway_epoch=client.epoch};
    assert(osg_cyphal_send(&gateway,&assignment,peer.now));
    assert(client.route_generation>route_generation && client.node==assignment.node && client.lease==assignment.lease);
    osg_module_config_can_tick(&can,peer.now);

    assert(!actions.rgb && !actions.registry.leases[0].owned && !can.action_route_valid && !can.cached);
    /* Old request IDs execute a fresh claim only after the old route cache is discarded. */
    request_now=5501;
    assert(can_command(&can,8,"action claim 1 led.rgb.set_color 00112233-4455-6677-8899-aabbccddeeff 5000 000000"));flush_response(&can);
    assert(strstr(can.response,"\"leaseId\":5"));
    assert(can_command(&can,10,"action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 5 1 led.rgb.set_color ffffff"));flush_response(&can);
    client.node=0;osg_module_config_can_tick(&can,5502);
    assert(!actions.rgb && !actions.registry.leases[0].owned && !can.action_route_valid);
    client.route_generation=UINT32_MAX;osg_can_client_reset(&client);
    assert(client.route_exhausted && client.exhausted && !client.node && client.route_generation==UINT32_MAX);
    peer.now=5600;assert(osg_cyphal_send(&gateway,&assignment,peer.now));assert(!client.node);
    osg_can_client_tick(&client,peer.now,(uint8_t[OSG_GW_MAX_REPORT_SIZE]){0});assert(!client.node);
    osg_cyphal_destroy(&gateway);
    assert(!runtime.owner && !runtime.pending_apply && !runtime.generation);
    osg_cyphal_destroy(&client.cyphal);request_now=1;
}
static void can_events_test(void) {
    osg_gateway_identity_t identity={{'O','S','G','E',1}};
    osg_gateway_profile_t profile={.button_count=1,.revision=1};
    osg_can_client_t client;assert(osg_can_client_init(&client,&identity,&profile,8,send_frame,NULL));
    assert(canard_set_node_id(&client.cyphal.canard,22));
    client.node=4;client.gateway_source=100;client.lease=20;client.epoch=2;
    osg_profile_button_t button={.input=0};
    osg_profile_t panel={.name="Master Caution",.buttons=&button,.button_count=1};
    osg_profile_sample_t sample={0};osg_events_t events;
    assert(osg_events_init(&events,&panel,&sample,0,1));osg_events_observe(&events,&sample,10);
    osg_runtime_config_t runtime={0};osg_module_config_can_t can;
    osg_module_config_can_init(&can,&client,&runtime);osg_module_config_can_events(&can,&events);
    assert(can_command(&can,1,"events state"));
    assert(!strncmp(can.response,"OK\n",3) && strstr(can.response,"\"head\":0"));
    char retained[sizeof can.response];strcpy(retained,can.response);flush_response(&can);
    sample.digital_closed=1;osg_events_observe(&events,&sample,20);osg_events_observe(&events,&sample,30);
    sample.digital_closed=0;osg_events_observe(&events,&sample,31);osg_events_observe(&events,&sample,41);
    assert(events.head==2);assert(can_command(&can,1,"events state"));
    assert(!strcmp(retained,can.response));flush_response(&can); /* Immutable read retry. */
    client.direct=true;
    assert(can_command(&can,2,"events read 1 0"));
    assert(strstr(can.response,"\"sequence\":1,\"control\":0,\"pressed\":true"));
    assert(strstr(can.response,"\"sequence\":2,\"control\":0,\"pressed\":false"));
    assert(!runtime.pending_apply && !runtime.owner && !runtime.generation);flush_response(&can);
    assert(osg_events_init(&events,&panel,&sample,50,2));
    assert(can_command(&can,3,"events read 1 2"));assert(strstr(can.response,"staleStream"));flush_response(&can);
    client.lease++;osg_module_config_can_tick(&can,100);
    assert(can_command(&can,4,"events state"));assert(strstr(can.response,"\"stream\":2"));flush_response(&can);
    osg_cyphal_destroy(&client.cyphal);
}
int main(void) { hardware_test(); lease_test(); simulation_test(); display_test(); calibrated_event_test(); can_action_test(); can_lease_test(); can_events_test(); return 0; }
