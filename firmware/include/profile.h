#pragma once
#include "opensimgear/profile.h"
const osg_profile_t *device_profile(void);
/* Called before USB enumeration. Invalid profiles fail before attachment. */
bool device_profile_prepare(void);
const uint8_t *device_report_descriptor(size_t *length);
const osg_profile_layout_t *device_report_layout(void);
