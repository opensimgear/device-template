# Home-milling device prototype

This is a separate two-layer bench layout of the canonical v0.1.0 circuit in the parent directory. It targets a Makera Z1 with the PCB toolkit and **unplated holes**. The additional area reserves accessible 1.80 mm wire lands for every MCU signal and the larger generic input banks. The assembled board is 105 × 125 mm; the routed blank is 110 × 125 mm including a removable 5 mm registration rail on its west side. USB and both powered-CAN sockets face the east edge. The nominal USB and RJ connector mouths align with the east PCB edge. Verify actual plug access, connector fit and strain clearance with purchased parts.

The layout keeps the original STM32G0B1CBT6 LQFP48, USB4105-GF-A, TCAN3413D and LM66100 component lands. Open signal runs use 0.30 mm tracks; local IC escapes, crowded routing lanes and USB use 0.20 mm. The final layout contains 4,586 mm of 0.30 mm copper and 1,395 mm of 0.20 mm copper; wider power routing is counted separately. Copper clearance is at least 0.20 mm. The editable stackup records two bare 35 µm copper faces and a nominal 1.53 mm FR4 core for a 1.60 mm laminate; actual copper/core thickness and dielectric properties must be measured or supplied by the laminate vendor. The nominal epsilon/loss values are metadata assumptions, not an impedance design. The 12 V pass-through is 1.00 mm. BUCK_5V uses 0.4002–0.50 mm, SYS_5V uses 0.20–0.50 mm including a short regulator escape, USB_VBUS uses 0.20–0.30 mm, and +3V3 uses 0.20–0.30 mm. Narrow pin transitions and the eventual measured power budget require populated-board checks. Dimensions describe nominal copper, not a verified machining result.

## Generic input connections

This layout implements the same eight analogue channels, 24 digital lines and local I²C expansion interface as the canonical board. Each device wires only the controls enabled by its firmware profile. Unused control connections are omitted from the advertised controller profile.

| Header | Pin assignment |
| --- | --- |
| J5, J7–J13 | Eight axes A0–A7; pin 1 +3V3, pin 2 signal, pin 3 GND |
| J14 | Pins 1–6 D0–D5; pin 7 GND |
| J15 | Pins 1–6 D6–D11; pin 7 GND |
| J16 | Pins 1–6 D12–D17; pin 7 GND |
| J17 | Pins 1–6 D18–D23; pin 7 GND |
| J18 | Pin 1 +3V3, pin 2 SDA, pin 3 SCL, pin 4 GND |

Axes use the canonical per-channel input resistor, filter capacitor and weak midpoint bias. Digital contact inputs have 10 kΩ pull-ups and are intended to close to GND. I²C is a short internal 3.3 V connection for optional 16-input boards, not an external cockpit bus. Do not apply 5 V or 12 V to analogue/digital inputs or use the USB/CAN supply pins as control inputs. See the [shared electrical design](../ELECTRICAL.md) and [pin allocation](../pin-allocation.json).

## Qualify the actual process first

Mill and populate the [qualification coupon](coupon/README.md) before this device. The USB and MCU footprints have 0.20 mm nominal gaps, leaving no process margin if the machine only just achieves that isolation. Test the cutter, board flatness, measured thickness, probing, opposite-face registration, isolation, pad adhesion and hand soldering with the actual laminate and purchased parts. Optional toolkit solder mask does not change the copper target or establish USB impedance.

The removable rail has two 2.00 mm NPTH registration holes, at board coordinates (77.5,64) and (77.5,181) mm. They are a layout starting point. Measure the actual fixture/dowel pins and adapt hole diameter and fit before generating CAM; the kit's pin diameter has not been established. Remove the rail at the user-drawing guide x=80 mm after both faces and drilling are complete. Mounting holes on the finished board remain separate.

## Unplated construction

Every electrical layer transition uses a **0.80 mm drilled hole with 1.80 mm copper lands**, joined by a wire soldered on both faces. These are represented by KiCad vias solely to model the completed hand-wired connection. The raw milled board has no conductive barrel. The [wire-join list](review/wire-joins.csv) records all **224** positions and nets. Complete and inspect every join before power-on. This assembled connectivity includes 1,701 copper track segments. The wire diameter and solder fillet must fit the 0.80 mm holes. Toolkit rivets are not assumed to fit: a rivet with 1.00 mm outside diameter does not fit this selected hole.

Long-leaded Evercom 5301-4P4C contacts, headers, buttons and the RECOM converter attach electrically on the **bottom face only**. Front track/zone keepouts around those component holes prevent a hidden need to solder under a connector or component body. Their front annuli are mechanically incidental and cannot bridge faces.

The USB4105 shell is different. Its nominal 0.95 mm stakes do not reach the back surface of 1.6 mm laminate. Solder its four shell lands from the **front**, making accessible fillets outside the case; the front shell ground must reach a verified ground wire join. Inspect fit, solder access and mechanical retention on the actual sample before committing a device. Back annuli do not turn short shell stakes into through leads. USB SMD contacts also solder on the front. C6–C9 (MCU decoupling) and C11 (USB power decoupling) are back-face SMD capacitors. C6–C9 sit directly beneath the MCU and connect to its accessible supply and ground wire joins; solder and inspect these back-face paths. Populate back-face parts only after both faces have been milled, drilled and inspected.

Retained drill/slot dimensions are distinct from the isolation cutter:

| Feature | Nominal hole/slot |
| --- | --- |
| Manual wire layer joins | 0.80 mm round |
| USB shell stakes | 0.60 × 1.70 and 0.60 × 1.40 mm slots |
| USB locating pegs | 0.65 mm round |
| Evercom 4P4C contacts / both locating pegs | 0.90 / 3.20 mm round |
| Samtec header leads | 1.02 mm round |
| Omron button leads | 1.10 mm round |
| RECOM converter leads | 1.00 mm round |
| Board mounting / rail registration | 3.20 / 2.00 mm round |

A 0.80 mm drill cannot produce the USB's 0.60 mm slots. Plan suitable small-slot tooling/CAM and inspect actual component fit. Do not enlarge connector slots or locating holes just to use one tool. Electrical holes here are unplated despite the canonical footprints' commercial plated-hole descriptions; the parent footprint/source review explains the nominal manufacturer geometry.

## Electrical and USB limits

This is a bench prototype, with a two-layer ground arrangement and manually soldered connections. It is **not** the 90 Ω controlled-impedance fabrication layout. USB leaves the MCU through accessible soldered-wire joins and continues on the back before returning to the front protection/connector routing. Neither this two-layer geometry, its extra wire transitions nor successful enumeration qualifies USB signal integrity; enumeration and signal quality require bench testing with the actual assembled prototype. Use the parent's JLCPCB four-layer layout and specified impedance process for that review.

The exact CAN jack is Evercom 5301-4P4C (LCSC C3097715), with an 11.10 × 18.05 × 11.45 mm body and a nose 14.25 mm from its footprint origin. Both 3.20 mm locating holes retain the independently checked drawing dimensions. Pin 1 is BUS_12V, pin 2 CAN_H, pin 3 CAN_L and pin 4 GND; both jacks are parallel connections to one linear bus.

The circuit and firmware pin mapping are shared with the canonical version: USB direct operation, two CAN pass-through connectors, 12 V alongside CAN, local protection/regulation and USB/bus power selection. The RJ sockets are powered CAN, not telephone or Ethernet ports. v0.1.0 firmware implements Cyphal/CAN discovery, gateway input delivery and direct USB ownership; see [CAN integration](../../../../../firmware/CAN_BRINGUP.md). Software tests and this PCB review do not establish physical hot-plug, ownership handover or USB/CAN behavior. Perform continuity, shorts, power-rail and supply handover checks before connecting a PC or CAN chain.

## Shared circuit and source refresh

Run `python3 regenerate_schematics.py` from this directory to regenerate both home schematic files from the parent v0.1.0 sources. The generator modifies only symbol Footprint properties using `source-footprints.json`; all references, part values, net names and circuit connectivity remain shared. HomeMilling footprint variants retain the exact component land/drill dimensions. The board arrangement and wire-via rules are specific to this prototype. The [parent purchasing BOM](../bom.csv) identifies the same components; its footprint column describes the production layout. Use the HomeMilling variants here for this board, rather than changing the purchasing parts.

Review exports and native KiCad reports are under `review/`: [schematic PDF](review/schematic.pdf), [two-face routing PDF](review/routing.pdf), [front copper](review/front.png), [back copper](review/back.png), [ERC](review/erc.rpt) and [DRC](review/drc.rpt). The final source has zero ERC violations, zero all-track DRC violations, zero unconnected items and zero schematic parity issues. A separate construction audit models the long-lead component pads as bottom-only and the four short USB shell lands as front-only; it retains the documented wire joins and also finds zero unconnected items. Its intentional diagnostic pad transformations produce expected library/pad-type warnings, not changes to the deliverable PCB. See the [unplated connectivity record](review/UNPLATED-CONNECTIVITY.md), [independent construction review](review/CONSTRUCTION-AUDIT.md) and `audit_unplated.py` for the precise checks. No CNC toolpaths or machine-ready milling files are supplied. CAM must be generated against the qualified machine, fixture, cutter and measured laminate.
