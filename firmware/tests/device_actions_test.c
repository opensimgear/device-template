#include "device_actions.h"
#include "config_can.h"
#include <assert.h>
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
        if(!osg_module_config_can_receive(s,&m,1)) return false;
    }
    return true;
}
static unsigned transmitted;
static bool send_frame(void *context,const osg_gateway_frame_t *frame) {
    (void)context; assert(frame->extended && frame->length); transmitted++; return true;
}
static void flush_response(osg_module_config_can_t *s) {
    assert(osg_module_config_can_busy(s));
    for(unsigned i=0;s->responding && i<256;i++) osg_module_config_can_tick(s,2+i);
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
int main(void) { hardware_test(); simulation_test(); can_action_test(); can_events_test(); return 0; }
