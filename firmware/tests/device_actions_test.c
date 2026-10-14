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
int main(void) { hardware_test(); simulation_test(); display_test(); calibrated_event_test(); can_action_test(); can_events_test(); return 0; }
