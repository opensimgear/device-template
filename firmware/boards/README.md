# Board targets

`starter_g0b1cb` is implemented for the custom [starter rev-a electronics](../../electronics/boards/starter/rev-a/README.md). It selects STM32G0B1CBT6, internal clocks, physical pins, SWD recovery and a 128 KiB / 144 KiB linker map. It is build-checked, not bench-validated. An unchanged Nucleo board is not this target.

Add a target for a changed PCB or pin mapping. Mechanical Lite/Pro editions can share a target when electronics agree. Gateway hardware remains undecided.
