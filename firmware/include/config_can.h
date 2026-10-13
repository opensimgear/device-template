#pragma once
#include "can_client.h"
#include "runtime_config.h"
#include "opensimgear/actions.h"
#include "opensimgear/events.h"
typedef struct {
    osg_can_client_t *client;
    osg_runtime_config_t *runtime;
    osg_actions_t *actions;
    osg_events_t *events;
    osg_gateway_message_t route;
    char command[OSG_GW_CONFIG_COMMAND_CAPACITY];
    char response[OSG_GW_CONFIG_RESPONSE_CAPACITY+4];
    uint16_t received,total,sent,response_size;
    uint32_t started;
    bool assembling,responding,cached,expected_apply;
} osg_module_config_can_t;
void osg_module_config_can_init(osg_module_config_can_t *,osg_can_client_t *,osg_runtime_config_t *);
/* Optional volatile device-command dispatcher, independent of configuration. */
void osg_module_config_can_actions(osg_module_config_can_t *,osg_actions_t *);
void osg_module_config_can_events(osg_module_config_can_t *,osg_events_t *);
bool osg_module_config_can_receive(osg_module_config_can_t *,const osg_gateway_message_t *,uint32_t now);
void osg_module_config_can_tick(osg_module_config_can_t *,uint32_t now);
bool osg_module_config_can_busy(const osg_module_config_can_t *);
