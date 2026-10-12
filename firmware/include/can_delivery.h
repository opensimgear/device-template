#pragma once
#include "can_client.h"
#include "opensimgear/profile.h"
typedef struct { uint8_t phase[OSG_MAX_ENCODERS]; uint32_t minimum[OSG_MAX_ENCODERS]; } osg_can_delivery_t;
void osg_can_delivery_observe(osg_can_delivery_t *,const osg_profile_state_t *,uint32_t next_sequence);
void osg_can_delivery_rearm(osg_can_delivery_t *);
/* Per-encoder phase guards prevent an older press ACK crediting a later press. */
void osg_can_delivery_completed(osg_can_delivery_t *,osg_profile_state_t *,uint32_t sequence,
    const uint8_t report[OSG_GW_MAX_REPORT_SIZE],uint32_t now);
bool osg_can_profile(const osg_profile_t *,const osg_profile_layout_t *,osg_gateway_profile_t *);
