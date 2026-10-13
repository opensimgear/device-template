#pragma once
#include "opensimgear/profile.h"
#include "gateway_journal.h"

enum { OSG_RUNTIME_NAME_CAPACITY=64, OSG_RUNTIME_COMMAND_CAPACITY=384,
       OSG_RUNTIME_RESPONSE_CAPACITY=4096, OSG_RUNTIME_CLAIM_MS=30000,
       OSG_RUNTIME_RECORD_SIZE=8+64+6+128*3+8*11+4*4+8*2+8*2 };
typedef struct { bool inverted; uint16_t deadzone; } osg_runtime_axis_t;
/* Owned storage. The pointer-based osg_profile_t view is never serialized. */
typedef struct {
    char name[OSG_RUNTIME_NAME_CAPACITY];
    uint8_t button_count,axis_count,hat_count,encoder_count,selector_count,expander_mask;
    osg_profile_button_t buttons[OSG_MAX_BUTTONS];
    osg_profile_axis_t axes[OSG_ADC_CHANNELS];
    osg_profile_hat_t hats[OSG_MAX_HATS];
    osg_profile_encoder_t encoders[OSG_MAX_ENCODERS];
    osg_profile_selector_t selectors[OSG_MAX_SELECTORS];
    osg_runtime_axis_t axis_options[OSG_ADC_CHANNELS];
} osg_runtime_definition_t;
typedef struct {
    osg_runtime_definition_t active,draft,defaults;
    osg_profile_t profile;
    osg_profile_layout_t layout;
    uint8_t descriptor[OSG_DESCRIPTOR_CAPACITY];
    size_t descriptor_length;
    osg_journal_io_t settings;
    uint32_t generation,saved_generation,owner,claim_expires_ms;
    bool persistence,pending_apply,dirty;
} osg_runtime_config_t;

/* NULL IO allows a volatile emulator. Erased IO first commits factory defaults;
 * a recognizable interrupted baseline is retried. Other invalid settings fail
 * closed, preserving evidence instead of silently replacing user settings. */
bool osg_runtime_init(osg_runtime_config_t *,const osg_profile_t *,const osg_journal_io_t *);
/* Complete factory definition, including axis inversion/deadzone. Stored user
 * settings take precedence; defaults remain the target of the reset command. */
bool osg_runtime_init_definition(osg_runtime_config_t *,const osg_runtime_definition_t *,const osg_journal_io_t *);
void osg_runtime_tick(osg_runtime_config_t *,uint32_t now);
void osg_runtime_invalidate_claim(osg_runtime_config_t *);
/* Returns NULL on success or a stable error token. Apply is completed only by
 * finish_apply after USB detachment and cancellation of old delivery state. */
const char *osg_runtime_command(osg_runtime_config_t *,const char *,uint32_t now,bool *apply_requested);
bool osg_runtime_finish_apply(osg_runtime_config_t *);
size_t osg_runtime_snapshot(const osg_runtime_config_t *,bool active_view,char *,size_t);
bool osg_runtime_encode(const osg_runtime_definition_t *,uint32_t generation,uint8_t *,size_t);
bool osg_runtime_decode(osg_runtime_definition_t *,uint32_t *generation,const uint8_t *,size_t);
/* Fresh axis reports retain firmware-common acquisition/freshness behavior.
 * Inversion and the rescaled center deadzone operate in unsigned HID units. */
void osg_runtime_transform_report(const osg_runtime_config_t *,const osg_profile_state_t *,uint8_t *);
size_t osg_runtime_utf16(const char *,uint16_t *,size_t);
