#include "device_actions.h"
#include <string.h>

static const char *const modes[]={"auto","on","off"};
static const osg_action_parameter_t mode_parameters[]={
    {.id="mode",.label="Mode",.type=OSG_ACTION_ENUM,.options=modes,.option_count=3}
};
static const osg_action_descriptor_t hardware_actions[]={
    {.id="led.status.set_mode",.label="Status LED",.endpoint="led.status",
     .semantics=OSG_ACTION_SETTER,.readable=true,.parameters=mode_parameters,.parameter_count=1}
};
static const osg_action_parameter_t rgb_parameters[]={
    {.id="color",.label="Color",.type=OSG_ACTION_RGB}
};
static const osg_action_parameter_t boolean_parameters[]={
    {.id="enabled",.label="Enabled",.type=OSG_ACTION_BOOLEAN}
};
static const osg_action_descriptor_t simulated_actions[]={
    {.id="led.rgb.set_color",.label="RGB LED 1",.endpoint="led.rgb",
     .semantics=OSG_ACTION_SETTER,.readable=true,.parameters=rgb_parameters,.parameter_count=1},
    {.id="led.boolean.set_enabled",.label="LED 2",.endpoint="led.boolean",
     .semantics=OSG_ACTION_SETTER,.readable=true,.parameters=boolean_parameters,.parameter_count=1}
};
static bool invoke(void *context,const char *id,const osg_action_value_t *values,size_t count) {
    osg_device_actions_t *s=context;
    if(count!=1) return false;
    if(!strcmp(id,"led.status.set_mode") && values[0].type==OSG_ACTION_ENUM) {
        for(unsigned i=0;i<3;i++) if(!strcmp(values[0].value.text,modes[i])) {
            s->status_mode=(osg_led_mode_t)i; return true;
        }
    } else if(!strcmp(id,"led.rgb.set_color") && values[0].type==OSG_ACTION_RGB) {
        s->rgb=values[0].value.rgb; return true;
    } else if(!strcmp(id,"led.boolean.set_enabled") && values[0].type==OSG_ACTION_BOOLEAN) {
        s->enabled=values[0].value.boolean; return true;
    }
    return false;
}
static bool read(void *context,const char *id,osg_action_value_t *values,size_t count) {
    osg_device_actions_t *s=context;
    if(count!=1) return false;
    if(!strcmp(id,"led.status.set_mode")) {
        values[0].type=OSG_ACTION_ENUM;
        strcpy(values[0].value.text,modes[s->status_mode]); return true;
    }
    if(!strcmp(id,"led.rgb.set_color")) {
        values[0].type=OSG_ACTION_RGB; values[0].value.rgb=s->rgb; return true;
    }
    if(!strcmp(id,"led.boolean.set_enabled")) {
        values[0].type=OSG_ACTION_BOOLEAN; values[0].value.boolean=s->enabled; return true;
    }
    return false;
}
bool osg_device_actions_init(osg_device_actions_t *s,bool simulated) {
    if(!s) return false;
    memset(s,0,sizeof *s);
    return osg_actions_init(&s->registry,simulated ? simulated_actions : hardware_actions,
        simulated ? sizeof simulated_actions/sizeof simulated_actions[0] : sizeof hardware_actions/sizeof hardware_actions[0],
        1,invoke,read,s);
}
bool osg_device_actions_status(const osg_device_actions_t *s,bool automatic,bool suspended) {
    return s && !suspended && (s->status_mode==OSG_LED_ON || (s->status_mode==OSG_LED_AUTO && automatic));
}
