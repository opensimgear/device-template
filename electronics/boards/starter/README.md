# Starter board

[Rev B](rev-b/README.md) is the generic STM32G0B1CBT6 starter: eight analogue headers, four banks of six digital lines and an I²C header for optional 16-input boards. Firmware profiles assign those resources to the controls each device needs. USB-C, both powered 4P4C CAN pass-through ports, regulation/power selection, SWD, reset and the status LED are retained.

Rev B has separate four-layer production and two-layer Makera Z1 layouts. [Rev A](rev-a/README.md) preserves the earlier example circuit, reviewed layouts and fine-pitch milling coupon. The electrical changes require the new `starter_g0b1cb_rev_b` firmware target; the Rev A firmware is retained at commit `5a78e46`. Rev A's six-contact CAN cabling is incompatible with Rev B's 4P4C standard. Physical qualification remains open.

This is a module starting point, independent of Lite/Pro mechanical editions. It does not select the two-port, single-chain gateway platform or qualify a final CAN cable.
