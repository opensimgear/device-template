#pragma once
#include "opensimgear/actions.h"

typedef enum { OSG_LED_AUTO, OSG_LED_ON, OSG_LED_OFF } osg_led_mode_t;
enum { OSG_DEVICE_DISPLAY_MAX=4 };
/* A fitted output port owns its immutable capabilities and driver state. Both
 * native and physical ports use these callbacks; software readback is not an
 * electrical measurement. Descriptor storage outlives the registry. */
typedef struct {
    const char *id,*label,*endpoint;
    osg_action_display_t display;
    uint8_t max_bytes;
    bool (*write)(void *context,const char *text);
    bool (*read)(void *context,char *text,size_t capacity);
    void *context;
} osg_device_display_port_t;
typedef struct {
    osg_actions_t registry;
    osg_led_mode_t status_mode;
    uint32_t rgb;
    bool enabled;
    const osg_device_display_port_t *displays;
    uint8_t display_count;
    osg_action_descriptor_t descriptors[2+OSG_DEVICE_DISPLAY_MAX];
    osg_action_parameter_t display_parameters[OSG_DEVICE_DISPLAY_MAX];
} osg_device_actions_t;

/* Hardware advertises only its fitted monochrome LED. The simulation supplies
 * an RGB LED and a boolean LED through the same production registry. */
bool osg_device_actions_init(osg_device_actions_t *,bool simulated);
bool osg_device_actions_init_outputs(osg_device_actions_t *,bool simulated_leds,
                                    const osg_device_display_port_t *,size_t count);
bool osg_device_actions_status(const osg_device_actions_t *,bool automatic,bool suspended);
