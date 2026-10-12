# Fabrication plans

The starter has one shared schematic and BOM, with separate layouts for commercial fabrication and home milling. No board has been fabricated or electrically validated, and no fabrication order has been placed. Static verification results are recorded in `VALIDATION.md`; they do not certify assembly, USB signal quality or hot plugging.

| Layout | Board and construction | Purpose and status |
| --- | --- | --- |
| Production, `starter.kicad_pcb` | 65 × 55 mm, four copper layers, nominal 1.6 mm | Compact layout; J1 USB and J2/J3 CAN mouths face the east edge. Uses the impedance construction below. |
| Home, `home-milling/starter.kicad_pcb` | 85 × 85 mm board plus alignment rail, two copper layers, 1.6 mm laminate | Separate Makera Z1 prototype layout and assembly plan, in progress. Not impedance qualified. Uses footprint-only schematic variants derived from the parent circuit; no independent electrical design is implied. |

## Production construction and USB

Specify **JLCPCB JLC04161H-7628**, four layers, nominal 1.6 mm, 1 oz outer / 0.5 oz inner copper, with the matching laminate and solder mask. Selecting only “1.6 mm FR-4” is insufficient. The [manufacturer's stackup](https://jlcpcb.com/impedance) gives:

| Layer | Material | Nominal thickness | εr |
| --- | --- | ---: | ---: |
| L1 / F.Cu | Outer copper | 0.035 mm | — |
| L1–L2 dielectric | 7628 prepreg | 0.2104 mm | 4.4 |
| L2 / In1.Cu | GND, inner copper | 0.0152 mm | — |
| Core | FR-4 | 1.065 mm | 4.6 |
| L3 / In2.Cu | Predominantly GND; local 12 V branch feed and CAN test spur | 0.0152 mm | — |
| L3–L4 dielectric | 7628 prepreg | 0.2104 mm | 4.4 |
| L4 / B.Cu | Outer copper | 0.035 mm | — |

The nominal copper/dielectric sum is 1.5862 mm before mask. The manufacturer's calculated finished thickness is 1.59 mm ±10%; retain the specified core and stackup identity instead of changing the core to force an exact 1.6000 mm sum.

The [official calculator](https://jlcpcb.com/pcb-impedance-calculator) was checked on 12 October 2026 with **Differential Pair, Non coplanar**, solder mask, signal **L1** and reference **L2**. Its 90 Ω result is **0.2474 mm trace width / 0.1501 mm edge gap**, giving 0.3975 mm centre spacing. The coupled main pair uses that geometry and a continuous L2 ground return. L3 is predominantly GND, with a 1 mm local BUS_12V branch feed and a short J6 CAN_L test spur; it is not an uninterrupted ground-only layer. These exceptions are outside the connector duplicate-contact bridge return corridor. Keep those short bottom bridges over clear L3 ground and stitch the ground layers locally at transitions. Keep unrelated front copper and tracks at least 1 mm beyond either outer pair edge; this corridor is a conservative layout choice, not a manufacturer-mandated calculator setting. Recalculate if the construction or nearby copper changes.

Short connector contact escapes and pad transitions are finite discontinuities; the 90 Ω calculation does not apply to every component pad. Keep duplicate USB-C data contacts joined near J1, maintain symmetric transitions, and avoid dangling test stubs. C11 and the USBLC6 ground return belong immediately beside U2. The MCU integrates USB pull-up and driver matching; additional series resistors are not an impedance correction. See [ST USB guidance](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html).

Request **90 Ω differential ±10% controlled impedance**, obtain the fabricator's approval of final etched geometry/construction, and request a **production impedance coupon and TDR report**. The calculator's 0.5% search setting is not a manufacturing tolerance. A coupon tests construction, while assembled connector/pad/via discontinuities still need USB testing. See [JLCPCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities/) and [impedance testing](https://jlcpcb.com/blog/pcb-impedance-testing).

## Production release checks

- Regenerate ERC, DRC and schematic/PCB parity checks after the final change; review any exception locally and preserve the reports. Check the actual released layout, not an earlier routed intermediate file.
- Review all copper, mask, silkscreen, outline and drill layers in the exported manufacturing viewer. Confirm no missing zones, return-plane breaks, isolated copper or unwanted edge copper.
- Specify plated contact holes and USB shell slots separately from NPTH locating/mounting holes. KiCad dimensions are **finished** hole/slot dimensions; the fabricator selects plating compensation. RJ12 contacts are 0.89 mm PTH, locating pegs 3.25 mm NPTH; USB locators are 0.65 mm NPTH and shell slots 0.60 mm wide PTH. See `FOOTPRINTS.md`.
- Compare a 1:1 print against purchased USB/RJ12 connectors, header posts and switches. Check contact order, pegs, nose/edge position, cable latch access, RECOM/C2 height, enclosure and screw clearances.
- Reconcile exact MPNs, quantities, polarity, pin 1 and orientation between schematic, BOM, board and assembly files. Approve substitutions against their own drawings and ratings.

## Home-milling qualification and assembly

The proposed general rules are **0.30 mm tracks / 0.20 mm copper clearance**, **0.80 mm drilled soldered-wire vias**, and **1.6 mm laminate**. The available equipment is a **Makera Z1 with the PCB manufacturing toolkit**. The [Makera PCB Fabrication Pack](https://global.makera.com/products/pcb-fabrication-pack) supports a double-sided workflow and optional solder-mask application; this design still assumes neither conductive hole barrels nor a qualified solder-mask process. Inventory the actual cutters, alignment pins and rivets supplied with your kit before CAM setup. Fine-pitch pads and local escapes require smaller features than the general track rule. The [Makera Z1 product specification](https://global.makera.com/products/makera-z1-desktop-cnc) lists PCB/FR-4 support, but does not establish a guaranteed clearance or double-sided registration capability for this board. Qualify the actual machine, stock and tools before machining the complete layout.

First mill the separate `home-milling/coupon/` qualification coupon: actual 0.50 mm-pitch STM32/USB lands, 0.65 mm-pitch LM66100 lands, 0.20/0.25/0.30 mm gaps, 0.30 mm tracks, double-sided registration and 0.80 mm wire vias. For a V tool, ideal cut width is `tip + 2 × depth × tan(angle/2)`: a 0.20 mm-tip 30° tool at 0.05 mm depth cuts about 0.227 mm, already wider than the 0.20 mm pad gap. A 0.10 mm-tip 30° tool at that depth gives about 0.127 mm; these are geometric estimates, not verified settings, and omit runout, warping and depth error. Follow the [Makera example's flatness/fixturing guidance](https://wiki.makera.com/Air/Manual/Air-LED-Light), then inspect and measure the coupon.

Home holes have no conductive barrel. Solder both ends of every specified wire via, with access retained outside component bodies. The [Makera tubular rivet](https://global.makera.com/products/pcb-rivets-200pcs) is 1 mm OD and does not fit the specified 0.80 mm holes; this plan uses wire rather than that rivet. The toolkit may contain other rivet sizes: measure their outside diameter, flange and length rather than treating the retail rivet size as an inventory of the kit. Changing to rivets requires rechecking hole/land dimensions and solder access. Route RJ12 contact connections on the bottom where the joints are accessible; its housing blocks top-side solder access. Check USB shell tabs, headers and converter pins for any required cross-face connection before fitting parts that obstruct access. Inspect isolation and continuity before powering up.

The home board's rear ground corridor and short paired USB routing are useful prototype measures. Its unknown laminate/etched geometry, absence of plated barrels and possible absence of solder mask cannot inherit the four-layer 90 Ω calculation. Successful USB enumeration is a bench result, not impedance qualification.

## Review manufacturing exports

`review/fabrication/` contains four-layer Gerbers and separate plated/non-plated Excellon drills generated from the statically checked production sources. They are a prototype review package, with no order placed. Apply the named construction and impedance requirements above when obtaining a quote; do not submit this package as generic two-layer artwork. The copper finish recorded in KiCad is ENIG. Nominal solder-mask thickness is 0.01524 mm per face; the manufacturer's final mask/etched construction governs the impedance result. KiCad's default loss-tangent entries are placeholders, not independently characterized laminate data or a USB margin calculation.
