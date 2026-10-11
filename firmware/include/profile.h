#pragma once
#include "opensimgear/button.h"
#include "opensimgear/inputs.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool button, toggle, encoder_push, selector_low, selector_high, axis_fresh;
    uint8_t hat_contacts, encoder_ab;
    uint16_t axis_raw;
} profile_sample_t;
typedef struct {
    osg_button_t button, encoder_push;
    osg_contact_t toggle, hat[4], selector[2];
    osg_encoder_t encoder;
    bool hat_armed, axis_seen, axis_available, hat_conflict, selector_conflict;
    uint16_t axis;
    uint32_t axis_at;
} profile_t;
void profile_init(profile_t *profile, const profile_sample_t *sample, uint32_t now);
void profile_rearm(profile_t *profile, const profile_sample_t *sample, uint32_t now);
void profile_update(profile_t *profile, const profile_sample_t *sample, uint32_t now, bool active, uint8_t report[4]);
void profile_neutral(uint8_t report[4]);
void profile_reported(profile_t *profile, const uint8_t report[4], uint32_t now);
