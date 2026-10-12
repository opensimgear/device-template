# Firmware validation

Run the native test commands in [firmware README](../README.md). A delivery-state test covers retained and canceled USB transfers, generation acknowledgement and resume readiness. All three supplied profiles have behavior and descriptor tests, including exact field omission and report length. Shared tests cover profile bounds, collisions, axis-only/button-only/hat-only descriptors, conflict groups, cross-bank faults, stale input and expansion freshness, time wraparound and acknowledged encoder pulses.

The CI cross-build matrix compiles `example`, `shifter` and `expanded` for `starter_g0b1cb_rev_b`. [Rev B validation](../../electronics/boards/starter/rev-b/VALIDATION.md) records CAD and physical gates. Native and compile checks do not replace physical USB, ADC, encoder timing or I²C fault tests.
