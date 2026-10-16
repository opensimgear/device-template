# Starter board

## Development status and disclaimer

This project is heavily under development and is not ready for use. Use it entirely at your own risk. To the fullest extent permitted by applicable law, the authors and contributors disclaim all liability for any damage, loss, or other issues arising from its use.

[Version 0.1.0](0.1.0/README.md) is the single current generic STM32G0B1CBT6 device design: eight analogue headers, four banks of six digital lines and an I²C header for optional 16-input boards. Firmware profiles assign those resources to each device's controls. USB-C, powered 4P4C CAN pass-through ports, regulation/power selection, SWD, reset and the status LED are retained.

Four-layer production and two-layer Makera Z1 fabrication variants share the current circuit and `starter_g0b1cb_v0_1_0` firmware pin map. The milling qualification coupon lives with the current home-milling layout. Earlier example circuits are available only in Git history. Physical qualification remains open.

This module is independent of Lite/Pro mechanical editions. Follow [PCB versioning](../../VERSIONING.md); keep 0.1.0 fixed until launch.
