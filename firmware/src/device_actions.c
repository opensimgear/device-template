#include "device_actions.h"
#include <stdio.h>
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
    for(unsigned i=0;i<s->display_count;i++) if(!strcmp(id,s->displays[i].id)) {
        return values[0].type==OSG_ACTION_STRING &&
            s->displays[i].write(s->displays[i].context,values[0].value.text);
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
    for(unsigned i=0;i<s->display_count;i++) if(!strcmp(id,s->displays[i].id)) {
        values[0].type=OSG_ACTION_STRING;
        return s->displays[i].read(s->displays[i].context,values[0].value.text,sizeof values[0].value.text);
    }
    return false;
}
bool osg_device_actions_init(osg_device_actions_t *s,bool simulated) {
    return osg_device_actions_init_outputs(s,simulated,NULL,0);
}
bool osg_device_actions_init_outputs(osg_device_actions_t *s,bool simulated,
                                    const osg_device_display_port_t *ports,size_t count) {
    if(!s) return false;
    memset(s,0,sizeof *s);
    if(count>OSG_DEVICE_DISPLAY_MAX || (count && !ports)) return false;
    unsigned base=simulated ? sizeof simulated_actions/sizeof simulated_actions[0] :
                              sizeof hardware_actions/sizeof hardware_actions[0];
    memcpy(s->descriptors,simulated ? simulated_actions : hardware_actions,base*sizeof s->descriptors[0]);
    for(unsigned i=0;i<count;i++) {
        if(!ports[i].write || !ports[i].read) return false;
        /* All builtin names are reserved, including LEDs absent from this
         * board variant: callback dispatch uses those names in every mode. */
        if(ports[i].id && (!strcmp(ports[i].id,"led.status.set_mode") ||
            !strcmp(ports[i].id,"led.rgb.set_color") || !strcmp(ports[i].id,"led.boolean.set_enabled"))) return false;
        s->display_parameters[i]=(osg_action_parameter_t){.id="text",.label="Text",.type=OSG_ACTION_STRING,.max_bytes=ports[i].max_bytes};
        s->descriptors[base+i]=(osg_action_descriptor_t){.id=ports[i].id,.label=ports[i].label,.endpoint=ports[i].endpoint,
            .semantics=OSG_ACTION_SETTER,.readable=true,.parameters=&s->display_parameters[i],.parameter_count=1,.display=&ports[i].display};
    }
    s->displays=ports;s->display_count=(uint8_t)count;
    return osg_actions_init(&s->registry,s->descriptors,base+count,1,invoke,read,s) &&
        osg_actions_enable_ownership(&s->registry);
}
bool osg_device_actions_legacy_led(osg_device_actions_t *s,const char *mode,uint32_t now) {
    if(!s || !mode) return false;
    char line[96],reply[OSG_ACTION_REPLY_CAPACITY];
    int length=snprintf(line,sizeof line,"action invoke %lu led.status.set_mode %s",(unsigned long)s->registry.catalogue_revision,mode);
    return length>0 && (size_t)length<sizeof line &&
        osg_actions_command_at(&s->registry,line,now,OSG_ACTION_SCOPE_USB,reply,sizeof reply) && strstr(reply,"\"ok\":true");
}
bool osg_device_actions_status(const osg_device_actions_t *s,bool automatic,bool suspended) {
    return s && !suspended && (s->status_mode==OSG_LED_ON || (s->status_mode==OSG_LED_AUTO && automatic));
}
