# Unplated connectivity verification

The final Rev B home PCB has 224 wire joins (1.80 mm copper lands / 0.80 mm drills), 1,701 track segments and zero missing connections in native KiCad. Its physical construction is checked separately because a commercial plated-hole model would conceal inaccessible layer transitions on a milled laminate.

`audit_unplated.py` creates a diagnostic copy under `.audit/`, leaving the fabrication source untouched. It changes 79 long-lead component contacts to bottom-only electrical contacts and the four short USB shell stakes to front-only contacts. All 224 explicit wire vias remain conductive between faces because assembly requires a wire soldered at both ends. The 247 front SMD pads and ten back SMD pads retain their soldered faces.

Native KiCad reports **zero unconnected items** after refilling the transformed model. The only diagnostic warnings are twenty library mismatches caused by those intentional contact transformations and four mechanical-pad warnings caused by representing the USB shell stakes as front electrical contacts. The audit rejects other finding types; this diagnostic copy is not a manufacturing board. The editable source itself reports **zero DRC violations and zero schematic parity issues**, without those pad transformations.

See [the physical-face report](unplated-connectivity.rpt), [construction metrics](construction.json), [wire-join coordinates](wire-joins.csv) and [independent construction audit](CONSTRUCTION-AUDIT.md). Copper clearance, per-pad isolation moats, exact manufacturer lands and all-face wire-solder access are separate physical checks in that independent audit.

These checks establish CAD connectivity and assembly access, not a qualified milling process, measured power budget or USB impedance. Mill the existing Rev A qualification coupon with the actual cutter, laminate and registration setup, inspect fit and solderability, then perform continuity and populated-board power/USB/CAN checks. No CNC toolpaths are included.
