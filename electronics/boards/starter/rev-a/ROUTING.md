# Rev A production routing record

The production layout is **65 × 55 mm, four copper layers**, with four M3 mounting holes. Its 3,575 mm² area is 58% smaller than the previous 100 × 85 mm layout. KiCad checks dated 2026-10-12 report zero ERC errors/warnings, zero DRC violations, zero unconnected items and zero schematic/PCB mismatches. This establishes static connectivity under the recorded rules; fabrication control and physical tests are separate requirements.

## Placement and layers

USB J1 and powered-CAN J2/J3 face the east edge at x=165.000 mm. J1's origin is (161.325, 144.500) mm; J2/J3 origins are (148.230, 113.000)/(148.230, 130.000) mm, all at 90°. Their nominal fronts align with the edge; J1's alignment follows GCT's recommended PCB-edge datum. Keep the physical outline distinct from component courtyards and confirm mating/latch access with purchased cables and the enclosure.

| Layer | Function |
| --- | --- |
| L1 / F.Cu | Main USB pair, local controls/supplies/CAN escapes, filled GND |
| L2 / In1.Cu | Continuous GND-reference plane; no routed signal or supply tracks |
| L3 / In2.Cu | Mostly GND, with local 1.00 mm BUS_12V feed and short J6 CAN_L branch |
| L4 / B.Cu | Remaining controls/CAN routing, duplicate USB contact bridges, filled GND |

The board contains **657 track segments and 103 through vias, including 47 GND vias**. Both outer pours and both inner ground planes are filled. L2 remains the main USB return reference; do not treat L3 as an uninterrupted plane because its local routed branches require clearances.

## USB geometry and return

USBLC6 U2 is immediately behind J1, with C11 and a short stitched ground return nearby. The main front-copper pair from U2 toward the MCU uses **0.2474 mm trace width / 0.1501 mm edge gap**, giving 0.3975 mm centre spacing along the parallel coupled run. This is the official JLCPCB calculator's 90 Ω result for the specified **JLC04161H-7628, solder-masked L1 over L2, noncoplanar differential pair**. Its 0.2104 mm L1–L2 dielectric and copper/material construction are recorded in [FABRICATION.md](FABRICATION.md). Request 90 Ω differential ±10% manufacturing control and a production coupon/TDR report; a calculator result is not a measured impedance.

Three front-copper rule areas preserve the main route's corridor: two exclude adjacent tracks/vias, and the enclosing area excludes front copper pours and vias. The corridor keeps unrelated copper approximately 1 mm beyond the outer pair edges along the coupled run. Pads and footprint placement remain allowed by these rule-area settings, so future edits must also preserve clearance manually. Short component launches and fanouts lie outside the uniform coupled geometry. The saved [reference audit](review/reference-audit.json) sampled 1,350 points on the main-width routes at approximately 0.05 mm intervals and found none without filled L2 GND beneath their centreline; the filled L2 plane is one region. This is a geometric check, not a signal-integrity measurement.

Measured planar copper lengths are:

| Path | D+ | D− |
| --- | ---: | ---: |
| U2 MCU-side pads 4/6 to U1 pads 34/33 | 33.14 mm | 33.00 mm |
| J1 primary A6/A7 contacts to U1 pads 34/33 | 41.66 mm | 40.65 mm |

Lengths follow connected track centreline paths and exclude via-barrel length and internal connector/IC structures. The two short B.Cu bridges join duplicate USB-C data contacts near J1, with two 0.50/0.30 mm vias per signal. The U2-to-MCU main path has no signal vias. Contact escapes use 0.15 mm tracks and local U2 fanout uses 0.20 mm; pads/launches are finite discontinuities to the calculated main geometry. No external USB pull-up or additional series matching resistor is fitted. Follow [ST USB guidance](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html) and validate the assembled USB interface.

## CAN, power and controls

J2/J3 are parallel ports on one linear CAN segment. CAN H/L use nominal 0.25 mm routing, with two short 0.1874 mm CAN_H fanout necks. Their pass-through is mainly on B.Cu with front escapes and short U3 branches. The J6 CAN_L test/termination spur uses two L3 segments totalling 8.66 mm; it does not interrupt L2's USB reference. No terminator is fitted on this intermediate module. Qualified twisted-pair cables, endpoint termination, chosen bitrate and external recovery still need system tests.

BUS_12V tracks are 1.00 mm, including the local L3 feed. Local BUS_FUSED, BUCK_IN, BUCK_5V and SYS_5V tracks are all 0.50 mm after the final width correction. F1 protects the local module branch; J2/J3 through power relies on separate gateway protection. U6/U7 LM66100 source selection, D1 polarity protection, D5 local TVS and regulators are grouped locally. Neither copper width nor device current ratings approve a connector/cable/chain current budget.

The 3.3 V net is explicitly assigned to **LogicSupply**, using 0.20 mm local routing and 0.15 mm fine-pitch escapes. USB_VBUS retains its separate 0.30 mm class with short 0.225/0.25/0.15 mm escapes. C1 and C13 sit beside the transceiver supply escapes and have adjacent ground vias. MCU decoupling has short supply connections and nearby ground returns. The ADC series resistor/filter are close to the MCU; remote controls use the outer layers. Firmware pin assignments and external-header numbering are unchanged.

## Recorded constraints

| Item | Geometry/rule |
| --- | --- |
| General controls | 0.20 mm tracks / 0.20 mm clearance |
| USB main pair | 0.2474 mm width / 0.1501 mm edge gap; USB class clearance 0.15 mm |
| USB contact and U2 fanout | 0.15 mm contact escapes / 0.20 mm fanout |
| CAN H/L | Nominal 0.25 mm; two CAN_H 0.1874 mm necks; 0.20 mm clearance |
| BUS_12V | 1.00 mm minimum track width |
| BUS_FUSED / BUCK_IN / BUCK_5V / SYS_5V | 0.50 mm minimum track width |
| LogicSupply / +3V3 | 0.20 mm class; local 0.15 mm pad escapes |
| USB_VBUS | 0.30 mm class; local 0.225/0.25/0.15 mm escapes |
| Normal through vias | 0.60 mm diameter / 0.30 mm finished drill |
| USB contact/short escapes | 0.50 mm diameter / 0.30 mm finished drill |
| Ground pours | 0.20 mm clearance; SMT direct / through-hole thermal connections |
| Through-hole thermals | 0.30 mm spokes / 0.25 mm gap |
| Board edge | 0.50 mm minimum copper clearance; zone boundaries inset 0.60 mm |
| Pad-to-hole | Explicit 0.15 mm rule for USB-C library geometry |

The project and `starter.kicad_dru` contain the net classes and minimum power-width rules. Preserve manufacturer lands: USB/LQFP adjacent pads have 0.20 mm nominal copper gaps, and the authored TI DCK lands have 0.25 mm. Finished plated holes/slots, NPTH, solder-mask registration and controlled impedance require the fabricator's construction review. No per-item routing exclusion hides a remaining violation.

## Reproduction and separate home layout

Critical paths and rule areas were set explicitly. Other outer-layer routing used [Freerouting 2.5.0](https://github.com/freerouting/freerouting/releases/tag/v2.5.0), followed by KiCad corrections and filled-copper checks. The editable PCB and project libraries are authoritative; router intermediates are not required.

[Front copper](review/pcb.svg), [back copper](review/pcb-back.svg) and [routing PDF](review/routing.pdf) use top-down coordinates. Rear copper is viewed through the board, rather than mirrored for bottom-side assembly. F.Fab identifies components and does not represent copper. See [VALIDATION.md](VALIDATION.md) for dated checks and remaining physical tests.

The separate `home-milling/` layout is in progress: an 85 × 85 mm, two-layer board with a 5 mm alignment rail, wire vias and a Makera Z1 qualification coupon. It shares the schematic/BOM, but has different geometry and assembly constraints and does not inherit the production impedance calculation. See [FABRICATION.md](FABRICATION.md).
