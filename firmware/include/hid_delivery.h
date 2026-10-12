#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t generation,pending_generation; bool inflight; } hid_delivery_t;
static inline void hid_delivery_invalidate(hid_delivery_t *state) { state->generation++; }
/* TinyUSB owns endpoint state. A software flag must never prevent reuse of an
 * endpoint TinyUSB declares free, nor allow a second submission while busy. */
static inline bool hid_delivery_ready(hid_delivery_t *state,bool endpoint_ready) {
    if(endpoint_ready) state->inflight=false;
    return endpoint_ready && !state->inflight;
}
static inline void hid_delivery_submitted(hid_delivery_t *state) {
    state->pending_generation=state->generation; state->inflight=true;
}
static inline bool hid_delivery_completed(hid_delivery_t *state,bool active) {
    state->inflight=false;
    return active && state->pending_generation==state->generation;
}
