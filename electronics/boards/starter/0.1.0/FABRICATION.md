# Fabrication plans

The starter has one shared schematic and BOM, with separate layouts for commercial fabrication and home milling. No board has been fabricated or electrically validated, and no fabrication order has been placed. Static verification results are recorded in `VALIDATION.md`; they do not certify assembly, USB signal quality or hot plugging.

| Layout | Board and construction | Purpose and status |
| --- | --- | --- |
| Production, `starter.kicad_pcb` | 85 × 75 mm, four copper layers, nominal 1.6 mm | Compact layout; J1 USB and J2/J3 4P4C CAN mouths face the east edge. Uses the impedance construction below. |
| Home, `home-milling/starter.kicad_pcb` | 105 × 125 mm board plus removable 5 mm rail, two copper layers, 1.6 mm laminate | Routed Makera Z1 prototype with accessible wire joins; see the final construction and validation records. Process qualification remains required. Uses footprint-only schematic variants derived from the parent circuit and the same purchasing BOM. Not impedance qualified. |

## Production construction and USB

Specify **JLCPCB JLC04161H-7628**, four layers, nominal 1.6 mm, 1 oz outer / 0.5 oz inner copper, with the matching laminate and solder mask. Selecting only “1.6 mm FR-4” is insufficient. The [manufacturer's stackup](https://jlcpcb.com/impedance) gives:

| Layer | Material | Nominal thickness | εr |
| --- | --- | ---: | ---: |
| L1 / F.Cu | Outer copper | 0.035 mm | — |
| L1–L2 dielectric | 7628 prepreg | 0.2104 mm | 4.4 |
| L2 / In1.Cu | GND reference with two local D12/D14 bridges outside the USB corridor | 0.0152 mm | — |
| Core | FR-4 | 1.065 mm | 4.6 |
| L3 / In2.Cu | GND pour with local 12 V branch feed, CAN logic/test and input routes | 0.0152 mm | — |
| L3–L4 dielectric | 7628 prepreg | 0.2104 mm | 4.4 |
| L4 / B.Cu | Outer copper | 0.035 mm | — |

The nominal copper/dielectric sum is 1.5862 mm before mask. The manufacturer's calculated finished thickness is 1.59 mm ±10%; retain the specified core and stackup identity instead of changing the core to force an exact 1.6000 mm sum.

The [official calculator](https://jlcpcb.com/pcb-impedance-calculator) was checked on 12 October 2026 with **Differential Pair, Non coplanar**, solder mask, signal **L1** and reference **L2**. Its 90 Ω result is **0.2474 mm trace width / 0.1501 mm edge gap**, giving 0.3975 mm centre spacing. The coupled main pair uses that geometry and a continuous L2 ground return. Two short D12/D14 input bridges occupy L2 outside the protected USB return area; retain their documented separation and filled-reference audit. L3 carries a GND pour, the retained local BUS_12V branch feed and J6 CAN_L test spur, plus v0.1.0 input and MCU CAN logic routes. It is not an uninterrupted ground-only layer. These exceptions are outside the connector duplicate-contact bridge return corridor. Keep those short bottom bridges over clear L3 ground and stitch the ground layers locally at transitions. Keep unrelated front copper and tracks at least 1 mm beyond either outer pair edge; this corridor is a conservative layout choice, not a manufacturer-mandated calculator setting. Recalculate if the construction or nearby copper changes.

Short connector contact escapes and pad transitions are finite discontinuities; the 90 Ω calculation does not apply to every component pad. Keep duplicate USB-C data contacts joined near J1, maintain symmetric transitions, and avoid dangling test stubs. C11 and the USBLC6 ground return belong immediately beside U2. The MCU integrates USB pull-up and driver matching; additional series resistors are not an impedance correction. See [ST USB guidance](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html).

Request **90 Ω differential ±10% controlled impedance**, obtain the fabricator's approval of final etched geometry/construction, and request a **production impedance coupon and TDR report**. The calculator's 0.5% search setting is not a manufacturing tolerance. A coupon tests construction, while assembled connector/pad/via discontinuities still need USB testing. See [JLCPCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities/) and [impedance testing](https://jlcpcb.com/blog/pcb-impedance-testing).

## Production release checks

- Regenerate ERC, DRC and schematic/PCB parity checks after the final change; review any exception locally and preserve the reports. Check the actual released layout, not an earlier routed intermediate file.
- Review all copper, mask, silkscreen, outline and drill layers in the exported manufacturing viewer. Confirm no missing zones, return-plane breaks, isolated copper or unwanted edge copper.
- Specify plated contact holes and USB shell slots separately from NPTH locating/mounting holes. KiCad dimensions are **finished** hole/slot dimensions; the fabricator selects plating compensation. 4P4C contacts are 0.90 mm PTH, locating pegs 3.20 mm NPTH; USB locators are 0.65 mm NPTH and shell slots 0.60 mm wide PTH. See `FOOTPRINTS.md`.
- Compare a 1:1 print against purchased USB/4P4C connectors, header posts and switches. Check contact order, pegs, nose/edge position, cable latch access, RECOM/C2 height, enclosure and screw clearances.
- Reconcile exact MPNs, quantities, polarity, pin 1 and orientation between schematic, BOM, board and assembly files. Approve substitutions against their own drawings and ratings.

## Home-milling qualification and assembly

The proposed general rules are **0.30 mm tracks / 0.20 mm copper clearance**, **0.80 mm drilled soldered-wire vias**, and **1.6 mm laminate**. The available equipment is a **Makera Z1 with the PCB manufacturing toolkit**. The [Makera PCB Fabrication Pack](https://global.makera.com/products/pcb-fabrication-pack) supports a double-sided workflow and optional solder-mask application; this design still assumes neither conductive hole barrels nor a qualified solder-mask process. Inventory the actual cutters, alignment pins and rivets supplied with your kit before CAM setup. Fine-pitch pads and local escapes require smaller features than the general track rule. The [Makera Z1 product specification](https://global.makera.com/products/makera-z1-desktop-cnc) lists PCB/FR-4 support, but does not establish a guaranteed clearance or double-sided registration capability for this board. Qualify the actual machine, stock and tools before machining the complete layout.

First mill the preserved [v0.1.0 qualification coupon](home-milling/coupon/README.md): actual 0.50 mm-pitch STM32/USB lands, 0.65 mm-pitch LM66100 lands, 0.20/0.25/0.30 mm gaps, 0.30 mm tracks, double-sided registration and 0.80 mm wire vias. For a V tool, ideal cut width is `tip + 2 × depth × tan(angle/2)`: a 0.20 mm-tip 30° tool at 0.05 mm depth cuts about 0.227 mm, already wider than the 0.20 mm pad gap. A 0.10 mm-tip 30° tool at that depth gives about 0.127 mm; these are geometric estimates, not verified settings, and omit runout, warping and depth error. Follow the [Makera example's flatness/fixturing guidance](https://wiki.makera.com/Air/Manual/Air-LED-Light), then inspect and measure the coupon.

Home holes have no conductive barrel. Solder both ends of every specified wire via, with access retained outside component bodies. The [Makera tubular rivet](https://global.makera.com/products/pcb-rivets-200pcs) is 1 mm OD and does not fit the specified 0.80 mm holes; this plan uses wire rather than that rivet. The toolkit may contain other rivet sizes: measure their outside diameter, flange and length rather than treating the retail rivet size as an inventory of the kit. Changing to rivets requires rechecking hole/land dimensions and solder access. Route 4P4C contact connections on the bottom where the joints are accessible; its housing blocks top-side solder access. Check USB shell tabs, headers and converter pins for any required cross-face connection before fitting parts that obstruct access. Inspect isolation and continuity before powering up.

The home board's rear ground corridor and short paired USB routing are useful prototype measures. Its unknown laminate/etched geometry, absence of plated barrels and possible absence of solder mask cannot inherit the four-layer 90 Ω calculation. Successful USB enumeration is a bench result, not impedance qualification.

## Review manufacturing exports

From the repository root, run `sh tools/bundle-jlcpcb.sh [version] [--output-dir directory] [--smd-only]`. The default version is the highest numeric MAJOR.MINOR.PATCH directory under `electronics/boards/starter/`. The script exports fresh Gerbers, separate PTH/NPTH drills, BOM and positions from the saved commercial native CAD using KiCad 10 and Python 3. Outputs default to workspace `.research/jlcpcb/starter-<version>-gerbers.zip`, `-bom.csv` and `-cpl.csv`; custom output paths are relative to the caller. The Gerber ZIP contains layers, drills and the project `LICENSE`, `THIRD_PARTY_NOTICES.md` and board `LICENSE-KICAD.md`. The same notices are also emitted beside the exports as `starter-<version>-LICENSE`, `starter-<version>-THIRD_PARTY_NOTICES.md` and `starter-<version>-LICENSE-KICAD.md`. Preserve these notices with the fabrication files; upload BOM/CPL separately for assembly.

The BOM groups exact value/package/MPN/catalog matches and preserves manufacturer part numbers. Optional schematic `LCSC` or `JLCPCB` fields supply catalog numbers; blank catalog numbers require matching the exact MPN in JLCPCB. The BOM and CPL use the same populated component set, excluding mounting holes, schematic BOM exclusions and DNP/position-file exclusions. Through-hole parts are included by default; `--smd-only` selects footprints marked SMD and adds `-smd` to output filenames. Coordinates retain KiCad’s absolute origin, in mm with positive Y upwards, matching the Gerber/drill export origin. Rotation is KiCad’s footprint rotation normalized to 0–360 degrees; no guessed component-specific correction is applied. Verify exact part matches, polarity, pin 1 and top/bottom orientation in JLCPCB’s assembly preview.

The script does not refill zones, run ERC/DRC, change source files or approve manufacture; complete the fabrication checks before ordering.

`review/fabrication/` contains four-layer Gerbers and separate plated/non-plated Excellon drills generated from the statically checked production sources. They are a prototype review package, with no order placed. Apply the named construction and impedance requirements above when obtaining a quote; do not submit this package as generic two-layer artwork. The copper finish recorded in KiCad is ENIG. Nominal solder-mask thickness is 0.01524 mm per face; the manufacturer's final mask/etched construction governs the impedance result. KiCad's default loss-tangent entries are placeholders, not independently characterized laminate data or a USB margin calculation.
