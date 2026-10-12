# Board targets

`starter_g0b1cb_rev_b` matches the [generic Rev B PCB](../../electronics/boards/starter/rev-b/README.md). It reserves USB, CAN, LED, VBUS sensing and SWD, and exposes eight analogue channels, 24 digital lines and PB6/PB7 I²C. Internal HSI/PLL provides 64 MHz; HSI48 uses USB SOF recovery. The linker uses 124 KiB application flash, reserves the last 4 KiB for the boot-counter journal and uses the guaranteed 128 KiB SRAM region, including 16 KiB stack and 512 bytes heap.

Rev A wiring is incompatible; its firmware is preserved at commit `5a78e46`. A requested `starter_g0b1cb` target fails clearly. Add a new board target when PCB pin assignments change. Mechanical Lite/Pro editions may share a board/profile when their electronics agree. The gateway has its own board port in [device-gateway](https://github.com/opensimgear/device-gateway), using the same MCU family. See [CAN integration](../CAN_BRINGUP.md) for boot-journal preservation and physical acceptance requirements.
