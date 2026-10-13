#pragma once
#include "opensimgear/actions.h"

typedef enum { OSG_LED_AUTO, OSG_LED_ON, OSG_LED_OFF } osg_led_mode_t;
typedef struct {
    osg_actions_t registry;
    osg_led_mode_t status_mode;
    uint32_t rgb;
    bool enabled;
} osg_device_actions_t;

/* Hardware advertises only its fitted monochrome LED. The simulation supplies
 * an RGB LED and a boolean LED through the same production registry. */
bool osg_device_actions_init(osg_device_actions_t *,bool simulated);
bool osg_device_actions_status(const osg_device_actions_t *,bool automatic,bool suspended);
