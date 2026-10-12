# Firmware validation

Run the native test commands in [firmware README](../README.md). A delivery-state test covers retained and canceled USB transfers, generation acknowledgement and resume readiness. All three supplied profiles have behavior and descriptor tests, including exact field omission and report length. Shared tests cover profile bounds, collisions, axis-only/button-only/hat-only descriptors, conflict groups, cross-bank faults, stale input and expansion freshness, time wraparound and acknowledged encoder pulses.

The CI cross-build matrix compiles `example`, `shifter` and `expanded` for `starter_g0b1cb_rev_b`. [Rev B validation](../../electronics/boards/starter/rev-b/VALIDATION.md) records CAD and physical gates. Native and compile checks do not replace physical USB, ADC, encoder timing or I²C fault tests.

The eighth native suite connects the device to real Cyphal peers and exercises assignment/timeout, lossless maximum profiles, exact gateway USB completion ACKs, gateway source binding, transport node-ID collision recovery, stale boot/lease/sequence/duplicate/wrong-report rejection, direct USB priority, per-encoder phase sequence guards and interrupted boot-counter journal writes. Profile behavior tests also compare each supplied profile with the generated Cyphal report length and exact neutral bytes. See [CAN validation](../CAN_BRINGUP.md).

The ninth suite runs the canonical shared transport tests against the device's
snapshot, including generated DSDL, FD/Classic transfers, node services,
collision recovery, queue deadlines and bounded allocation.
