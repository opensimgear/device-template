# Board targets

`starter_g0b1cb_rev_b` matches the [generic Rev B PCB](../../electronics/boards/starter/rev-b/README.md). It reserves USB, CAN, LED, VBUS sensing and SWD, and exposes eight analogue channels, 24 digital lines and PB6/PB7 I²C. Internal HSI/PLL provides 64 MHz; HSI48 uses USB SOF recovery. The linker map is 128 KiB flash / 144 KiB RAM.

Rev A wiring is incompatible; its firmware is preserved at commit `5a78e46`. A requested `starter_g0b1cb` target fails clearly. Add a new board target when PCB pin assignments change. Mechanical Lite/Pro editions may share a board/profile when their electronics agree. The gateway remains a separate platform.
