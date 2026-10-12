#include "hid_delivery.h"
#include <assert.h>
int main(void) {
    hid_delivery_t state={0};
    assert(hid_delivery_ready(&state,true)); hid_delivery_submitted(&state);
    assert(!hid_delivery_ready(&state,false));
    /* A retained USB packet stays busy through suspend/resume. Its eventual
     * completion cannot acknowledge a newly rearmed encoder generation. */
    hid_delivery_invalidate(&state); assert(!hid_delivery_ready(&state,false));
    hid_delivery_invalidate(&state); assert(!hid_delivery_completed(&state,true));
    assert(hid_delivery_ready(&state,true)); hid_delivery_submitted(&state);
    assert(hid_delivery_completed(&state,true));
    /* If reset/cancellation makes the endpoint free without a completion,
     * the old software flag cannot freeze all future reports. */
    hid_delivery_submitted(&state); hid_delivery_invalidate(&state);
    assert(hid_delivery_ready(&state,true)); hid_delivery_submitted(&state);
    assert(!hid_delivery_completed(&state,false));
    return 0;
}
