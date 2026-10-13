# v0.1.0 production routing record

The generic production board is **85 × 75 mm with four copper layers** and four M3 mounting holes. It carries eight filtered/bias-equipped analogue channels, 24 digital pull-ups and a local I²C expansion header. USB and both powered CAN connectors face the east edge. Their footprint lands retain manufacturer geometry; board outline, connector noses and enclosure access must be checked against purchased samples.

## Placement and layers

| Layer | Function |
| --- | --- |
| L1 / F.Cu | Main USB pair, local component/power connections, input escapes and GND pour |
| L2 / In1.Cu | GND reference plane with two local D12/D14 GPIO bridges outside the USB return corridor |
| L3 / In2.Cu | GND pour with input/CAN logic routes, local 12 V feed and CAN test spur |
| L4 / B.Cu | Remaining input/CAN routes, duplicate USB contact bridges and GND pour |

L2 is the main USB return reference. Two short, explicit D12/D14 bridges resolve enclosed input routes away from USB; no general signal routing is permitted on this layer. L3 carries signals and power outside the protected duplicate-contact bridge return corridor; it is not an uninterrupted plane. Preserve local stitching and the ground returns beside the MCU, USB protection and transceiver.

The final layout has **3,757 track segments and 247 vias**, including 64 ground vias: 1,360 front segments, 1,294 back, 1,006 on L3 and 97 on L2. Only D12 and D14 occupy L2: their routes are 17.045 mm and 8.315 mm long, respectively, with minimum planar copper-to-USB separation of **4.269 mm / 1.175 mm**. Filled L2 GND remains one connected region. Exact routes and source hashes are recorded in [metrics](review/metrics.json) and the [independent reference audit](review/production-usb-reference-review.json).

## USB geometry and return

The main front-copper pair uses **0.2474 mm width / 0.1501 mm edge gap**, giving 0.3975 mm centre spacing. This is the JLCPCB calculator's 90 Ω result for the specified **JLC04161H-7628, solder-masked L1 over L2, noncoplanar differential pair**. Its construction is documented in [FABRICATION.md](FABRICATION.md). Request 90 Ω differential ±10% manufacturing control and a coupon/TDR report. The calculator result is not a measured impedance.

Rule areas exclude unrelated front copper, tracks and vias from the main coupled-route corridor. Short pad escapes and component launches are finite discontinuities outside the uniform geometry. USBLC6 U2, its C11 bypass and the stitched ground return remain directly behind J1. The duplicate USB-C contacts join near J1 over short back-copper bridges; preserve clear L3 ground beneath those bridges. The MCU-side main path remains on front copper, without signal vias. No external USB pull-up or additional series matching resistors are fitted.

The independent audit passes all **11 assertions**: all 37 reviewed front USB segments at their full widths/endcaps and all differential component/connector launch pads lie over filled L2 GND. The duplicate back bridges are **1.61245 mm DM / 1.600 mm DP**; their central full widths lie over L3 GND outside the unavoidable individual signal-via antipads. Each nominal 0.45 mm-radius antipad is bounded by a further 0.005 mm native polygon allowance; the audit does not exclude whole merged holes. These finite launch discontinuities are not controlled-impedance segments. Physical USB performance still requires assembled-board testing.

## CAN, power and input routing

J2/J3 are parallel **4P4C** ports on one linear powered CAN segment: pin 1 BUS_12V, 2 CAN_H, 3 CAN_L, 4 GND. This pinout is incompatible with the historical six-contact circuit. Endpoint plugs place 120 Ω between contacts 2 and 3; an intermediate module has no fitted terminator. Premade cords must be qualified for contact mapping, twisting, resistance and current; ordinary handset cords are not automatically suitable.

CAN_H/CAN_L retain their physical bus routing and transceiver branches. The separate MCU CAN RX/TX/standby logic may route on L3. BUS_12V uses 1.00 mm tracks; local BUS_FUSED, BUCK_IN, BUCK_5V and SYS_5V use 0.50 mm. F1 protects only the local module branch, while connector pass-through relies on gateway protection. Copper widths do not establish the allowable whole-chain current.

The ADC resistor/filter/bias networks and digital pull-ups connect the new banks without changing the documented MCU pin assignment. The GPIOFanout class uses **0.15 mm tracks / 0.15 mm clearance and 0.50/0.25 mm plated vias** for commercial input, MCU CAN logic, SWD/reset and LED escapes. This class does not apply to the two-layer home layout. Supply, USB and CAN_H/L classes retain their separate geometry and minimum widths.

## Recorded constraints

| Item | Geometry/rule |
| --- | --- |
| General routing | 0.20 mm tracks / 0.20 mm clearance |
| GPIOFanout | 0.15 mm tracks / 0.15 mm clearance; 0.50 mm via / 0.25 mm drill |
| USB main pair | 0.2474 mm width / 0.1501 mm gap; USB class clearance 0.15 mm |
| USB contact and U2 fanout | 0.15 mm contact escapes / 0.20 mm fanout |
| CAN H/L | Nominal 0.25 mm, with reviewed local necks |
| BUS_12V | 1.00 mm minimum track width |
| Local fused/regulated 5 V branches | 0.50 mm minimum track width |
| LogicSupply / +3V3 | 0.20 mm class, local 0.15 mm pad escapes |
| USB_VBUS | 0.30 mm class, reviewed local escapes |
| Through vias | 130 at 0.60/0.30 mm, 106 at 0.50/0.25 mm and 11 retained local fanout vias at 0.50/0.30 mm (land/drill) |
| Ground pours | 0.20 mm clearance; SMT direct / through-hole thermal connections |
| Through-hole thermals | Normally 0.30 mm spokes / 0.25 mm gap; J11 0.20/0.20 mm and J18 0.15/0.15 mm at 45°, minimum two spokes |
| Board edge | 0.50 mm minimum copper clearance; zones inset 0.60 mm |
| USB-C pad-to-hole | Explicit 0.15 mm rule for manufacturer footprint geometry |

The project and `starter.kicad_dru` define these constraints. Do not reduce manufacturer lands to fit routes. Finished plated holes/slots, non-plated holes, mask registration and the new fanout class need fabricator review.

## Review and home construction

Critical paths and fanouts were set explicitly. Other routing used [Freerouting 2.5.0](https://github.com/freerouting/freerouting/releases/tag/v2.5.0), followed by native KiCad filled-copper, all-track and schematic-parity checks. Editable KiCad sources and libraries are authoritative; router intermediates are not needed.

[Front copper](review/pcb.svg), [back copper](review/pcb-back.svg) and [routing PDF](review/routing.pdf) use top-down coordinates; rear copper is viewed through the board, not mirrored for assembly. F.Fab identifies components and does not represent copper.

The separate [home layout](home-milling/README.md) has a **105 × 125 mm body plus a removable 5 mm alignment rail**. Its 0.30 mm general tracks, 0.20 mm local escapes/isolation and accessible 1.80/0.80 mm wire joins serve the Makera Z1 prototype process. It uses the same circuit and purchasing BOM with footprint-only variants for unplated construction. It has its own connectivity and solder-access checks and cannot inherit the four-layer impedance calculation. See [VALIDATION.md](VALIDATION.md) for static results and remaining physical tests.
