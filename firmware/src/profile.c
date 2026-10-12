#include "profile.h"
#ifndef OSG_DEVICE_PROFILE_HEADER
#define OSG_DEVICE_PROFILE_HEADER "example.h"
#endif
#include OSG_DEVICE_PROFILE_HEADER
static uint8_t descriptor[OSG_DESCRIPTOR_CAPACITY];
static size_t descriptor_length;
static osg_profile_layout_t layout;
const osg_profile_t *device_profile(void) { return &selected_profile; }
bool device_profile_prepare(void) {
    if(osg_profile_validate(&selected_profile,&layout)!=OSG_PROFILE_OK) return false;
    descriptor_length=osg_profile_descriptor(&selected_profile,descriptor,sizeof descriptor);
    return descriptor_length!=0;
}
const uint8_t *device_report_descriptor(size_t *length) { *length=descriptor_length; return descriptor; }
const osg_profile_layout_t *device_report_layout(void) { return &layout; }
