# Firmware validation

Run the native test commands in [firmware README](../README.md). A delivery-state test covers retained and canceled USB transfers, generation acknowledgement and resume readiness. All three supplied profiles have behavior and descriptor tests, including exact field omission and report length. Shared tests cover profile bounds, collisions, axis-only/button-only/hat-only descriptors, conflict groups, cross-bank faults, stale input and expansion freshness, time wraparound and acknowledged encoder pulses.

The CI cross-build matrix compiles `example`, `shifter` and `expanded` for `starter_g0b1cb_v0_1_0`. [v0.1.0 validation](../../electronics/boards/starter/0.1.0/VALIDATION.md) records CAD and physical gates. Native and compile checks do not replace physical USB, ADC, encoder timing or I²C fault tests.

The CAN client suite connects the device to real Cyphal peers and exercises assignment/timeout, lossless maximum profiles, exact gateway USB completion ACKs, gateway source binding, transport node-ID collision recovery, stale boot/lease/sequence/duplicate/wrong-report rejection, direct USB priority, per-encoder phase sequence guards and interrupted boot-counter journal writes. Profile behavior tests also compare each supplied profile with the generated Cyphal report length and exact neutral bytes. See [CAN validation](../CAN_BRINGUP.md).

The shared transport suite runs the canonical tests against the device's
snapshot, including generated DSDL, FD/Classic transfers, node services,
collision recovery, queue deadlines and bounded allocation.

The runtime configuration suite runs the production core with simulated flash. It covers every control kind, complete-profile validation, writer conflict/expiry/wraparound, draft discard/reset, pending versus active descriptors, calibration/inversion/deadzone, maximum snapshots, canonical storage and corrupt settings. Initialization commits factory defaults as saved generation 1; tests interrupt each of its 80 erase/program operations, including a second interruption while recovering, then reboot and retry. Both first-user `apply_save` and `save` are interrupted at each of their 80 operations and preserve the baseline on reboot. Invalid committed records are preserved and rejected. Each profile HID suite also switches to a runtime axis-only descriptor through the actual USB callbacks and checks Unicode UTF-16 interface strings. Hardware flash/USB timing remains outside these tests.
