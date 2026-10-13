# Home-milling footprint interpretation

Component identities and manufacturer geometry are documented in the [canonical footprint review](../FOOTPRINTS.md) and [BOM](../bom.csv). `HomeMilling.pretty/` retains the same component copper lands and holes; its per-reference identifiers allow KiCad to compare this derived layout with its derived schematics. Rail alignment holes HA1/HA2 are the only extra board mechanics.

Commercial descriptions of plated contacts/shell slots do not apply to the home construction. Long component leads have back-only electrical attachment and isolated front annuli. USB4105 short shell stakes solder on the front and are grounded there. Every layer transition is a wire through a 0.80 mm hole with 1.80 mm lands, soldered on both faces. See [assembly and tooling details](README.md). No conductive barrel is assumed.

The physical pad dimensions, 0.20 mm USB/LQFP gaps and 0.25 mm LM66100 gaps have not been widened or reduced. Process qualification on the [coupon](coupon/README.md) must establish actual milled isolation and solderability. Exact connector slots/drills and front shell access remain sample-fit checks.

The generic input-bank variants include the reviewed TSW-103-07-L-S three-pin axis headers, four TSW-107-07-L-S seven-pin digital banks and the TSW-104-07-L-S local I²C header. All retain 1.02 mm nominal holes and 1.70 mm copper lands. Each physical pad instance receives its own front isolation area; repeated switch pad numbers do not share an assumed moat.

The CAN sockets are actual Evercom 5301-4P4C parts (LCSC C3097715). Their derived variants preserve the independently checked production lands: four 0.90 mm contact drills, 1.80 mm contact copper lands and two 3.20 mm locating holes on a 7.50 mm span. The contact positions are 1=(0,0), 2=(1.27,−2.54), 3=(2.54,0), 4=(3.81,−2.54) mm; the body nose lies +14.25 mm along the footprint's local Y axis. Both installed sockets rotate 90° and align their nose with the east edge. The contact numbering is component-side numbering; wiring is 1=BUS_12V, 2=CAN_H, 3=CAN_L, 4=GND.

C6–C9 and C11 retain the same 0805 lands as the canonical circuit but are assembled on the back face. Copper/drawing layer reversal is an assembly transformation; it does not alter the selected component geometry.
