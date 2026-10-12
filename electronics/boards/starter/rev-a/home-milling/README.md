# Home-milling device prototype

This is a separate two-layer bench layout of the canonical Rev A circuit in the parent directory. It targets a Makera Z1 with the PCB toolkit and **unplated holes**. The assembled board is 85 × 85 mm; the routed blank is 90 × 85 mm including a removable 5 mm registration rail on its west side. USB and both powered-CAN sockets face the east edge. The nominal USB and RJ connector mouths align with the east PCB edge. Verify actual plug access, connector fit and strain clearance with purchased parts.

The layout keeps the original STM32G0B1CBT6 LQFP48, USB4105-GF-A, TCAN3413D and LM66100 component lands. Ordinary signal traces are 0.30 mm; local IC escapes and USB are 0.20 mm. Copper clearance is at least 0.20 mm. The 12 V pass-through is 1.00 mm. BUCK_5V uses 0.4002–0.50 mm, SYS_5V uses 0.20–0.50 mm including a short regulator escape, USB_VBUS uses 0.20–0.30 mm, and +3V3 uses 0.20 mm. Narrow pin transitions and the eventual measured power budget require populated-board checks. Dimensions describe nominal copper, not a verified machining result.

## Qualify the actual process first

Mill and populate the [qualification coupon](coupon/README.md) before this device. The USB and MCU footprints have 0.20 mm nominal gaps, leaving no process margin if the machine only just achieves that isolation. Test the cutter, board flatness, measured thickness, probing, opposite-face registration, isolation, pad adhesion and hand soldering with the actual laminate and purchased parts. Optional toolkit solder mask does not change the copper target or establish USB impedance.

The removable rail has two 2.00 mm NPTH registration holes, at board coordinates (97.5,104) and (97.5,181) mm. They are a layout starting point. Measure the actual fixture/dowel pins and adapt hole diameter and fit before generating CAM; the kit's pin diameter has not been established. Remove the rail at the user-drawing guide x=100 mm after both faces and drilling are complete. Mounting holes on the finished board remain separate.

## Unplated construction

Every electrical layer transition uses a **0.80 mm drilled hole with 1.80 mm copper lands**, joined by a wire soldered on both faces. These are represented by KiCad vias solely to model the completed hand-wired connection. The raw milled board has no conductive barrel. The supplied via list records their positions and nets; complete and inspect all **120 wire joins** before power-on. The wire diameter and solder fillet must fit the 0.80 mm holes. Toolkit rivets are not assumed to fit: a rivet with 1.00 mm outside diameter does not fit this selected hole.

Long-leaded RJ12 contacts, headers, buttons and the RECOM converter attach electrically on the **bottom face only**. Front track/zone keepouts around those component holes prevent a hidden need to solder under a connector or component body. Their front annuli are mechanically incidental and cannot bridge faces.

The USB4105 shell is different. Its nominal 0.95 mm stakes do not reach the back surface of 1.6 mm laminate. Solder its four shell lands from the **front**, making accessible fillets outside the case; the front shell ground must reach a verified ground wire join. Inspect fit, solder access and mechanical retention on the actual sample before committing a device. Back annuli do not turn short shell stakes into through leads. USB SMD contacts also solder on the front. C11 is the sole back-face SMD capacitor; populate it after registration/milling and inspect its local VBUS/ground attachment.

Retained drill/slot dimensions are distinct from the isolation cutter:

| Feature | Nominal hole/slot |
| --- | --- |
| Manual wire layer joins | 0.80 mm round |
| USB shell stakes | 0.60 × 1.70 and 0.60 × 1.40 mm slots |
| USB locating pegs | 0.65 mm round |
| RJ12 contacts / locating pegs | 0.89 / 3.25 mm round |
| Samtec header leads | 1.02 mm round |
| Omron button leads | 1.10 mm round |
| RECOM converter leads | 1.00 mm round |
| Board mounting / rail registration | 3.20 / 2.00 mm round |

A 0.80 mm drill cannot produce the USB's 0.60 mm slots. Plan suitable small-slot tooling/CAM and inspect actual component fit. Do not enlarge connector slots or locating holes just to use one tool. Electrical holes here are unplated despite the canonical footprints' commercial plated-hole descriptions; the parent footprint/source review explains the nominal manufacturer geometry.

## Electrical and USB limits

This is a bench prototype, with a two-layer ground arrangement and manually soldered connections. It is **not** the 90 Ω controlled-impedance fabrication layout. Its front USB route and retained back reference corridor support functional testing, but neither trace geometry nor successful enumeration qualifies USB signal integrity. Use the parent's JLCPCB four-layer layout and specified impedance process for that review.

The circuit and firmware pin mapping are shared with the canonical version: USB direct operation, two CAN pass-through connectors, 12 V alongside CAN, local protection/regulation and USB/bus power selection. The RJ sockets are powered CAN, not telephone or Ethernet ports. CAN/Cyphal discovery, hot plugging and duplicate-input arbitration remain firmware integration work in the starter scaffold; this PCB review does not establish their implementation. These functions still require populated-board tests. Perform continuity, shorts, power-rail and supply handover checks before connecting a PC or CAN chain.

## Shared circuit and source refresh

Run `python3 regenerate_schematics.py` from this directory to regenerate both home schematic files from the parent Rev A sources. The generator modifies only symbol Footprint properties using `source-footprints.json`; all references, part values, net names and circuit connectivity remain shared. HomeMilling footprint variants retain the exact component land/drill dimensions. The board arrangement and wire-via rules are specific to this prototype. The [parent purchasing BOM](../bom.csv) identifies the same components; its footprint column describes the production layout. Use the HomeMilling variants here for this board, rather than changing the purchasing parts.

Review exports and native KiCad reports are under `review/`. The routed source passes native KiCad ERC, DRC, connectivity and schematic-parity checks with zero issues. A separate construction audit models the 48 long-lead component pads as bottom-only and the four short USB shell lands as front-only; it retains the 120 wire joins and also finds zero unconnected items. Its intentional diagnostic pad transformations produce expected library/pad-type warnings, not changes to the deliverable PCB. See the [unplated connectivity record](review/UNPLATED-CONNECTIVITY.md), [independent construction review](review/CONSTRUCTION-AUDIT.md) and `audit_unplated.py` for the precise checks. No CNC toolpaths or machine-ready milling files are supplied. CAM must be generated against the qualified machine, fixture, cutter and measured laminate.
