#pragma once
#include "opensimgear/cyphal.h"

enum { OSG_CAN_HISTORY=16, OSG_CAN_ANNOUNCE_MS=500, OSG_CAN_STATUS_MS=500, OSG_CAN_INPUT_MS=10,
       OSG_CAN_ASSIGN_TIMEOUT_MS=2000 };
typedef bool (*osg_can_send_t)(void *,const osg_gateway_frame_t *);
typedef struct { uint32_t sequence; uint8_t report[OSG_GW_MAX_REPORT_SIZE]; } osg_can_sent_t;
typedef struct {
    osg_cyphal_t cyphal;
    osg_gateway_identity_t identity;
    osg_gateway_profile_t profile;
    osg_can_send_t send; void *context;
    uint32_t boot,lease,epoch,next_sequence,ack_sequence,generation,configuration_generation;
    uint32_t announced_at,announce_due,random,input_at,assigned_at,ack_at,status_at;
    uint8_t node,gateway_source,history_head;
    bool direct,announced,announce_waiting,input_due,exhausted,status_due;
    osg_can_sent_t history[OSG_CAN_HISTORY];
    uint8_t acknowledged[OSG_GW_MAX_REPORT_SIZE];
    uint32_t rejected,tx_dropped;
    bool (*config_receive)(void *,const osg_gateway_message_t *,uint32_t);
    void *config_context;
} osg_can_client_t;
bool osg_can_client_init(osg_can_client_t *,const osg_gateway_identity_t *,
    const osg_gateway_profile_t *,uint32_t boot,osg_can_send_t,void *);
/* Return true when ownership changed and acquisition must rearm. */
bool osg_can_client_usb(osg_can_client_t *,bool direct);
void osg_can_client_reset(osg_can_client_t *);
bool osg_can_client_active(const osg_can_client_t *);
void osg_can_client_tick(osg_can_client_t *,uint32_t now,const uint8_t report[OSG_GW_MAX_REPORT_SIZE]);
/* True only for a new, exact, current lease/session report completed over gateway USB. */
bool osg_can_client_receive(osg_can_client_t *,const osg_gateway_frame_t *,uint32_t now);
