#include "profile.h"
#ifndef OSG_DEVICE_PROFILE_HEADER
#define OSG_DEVICE_PROFILE_HEADER "example.h"
#endif
#include OSG_DEVICE_PROFILE_HEADER
static osg_runtime_config_t runtime;
const osg_profile_t *device_profile(void) { return &runtime.profile; }
bool device_profile_prepare(void) {
    return osg_runtime_init(&runtime,&selected_profile,NULL);
}
bool device_profile_load(const osg_journal_io_t *io) { return osg_runtime_init(&runtime,&selected_profile,io); }
osg_runtime_config_t *device_runtime_config(void) { return &runtime; }
const uint8_t *device_report_descriptor(size_t *length) { *length=runtime.descriptor_length; return runtime.descriptor; }
const osg_profile_layout_t *device_report_layout(void) { return &runtime.layout; }
