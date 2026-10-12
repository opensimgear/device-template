# Unplated construction connectivity check

The delivered `starter.kicad_pcb` passes its ordinary native KiCad DRC with **zero violations, zero missing connections and zero schematic-parity issues**. The schematic ERC also has zero violations.

Ordinary KiCad PTH pads model conductive barrels. This home board has none, so a second diagnostic verifies the intended soldered faces rather than relying on that assumption:

- The 48 long-lead component pads become electrical contacts on B.Cu only.
- The four short USB4105 SH lands become electrical contacts on F.Cu only.
- The 169 front and two back SMD contacts stay on their actual faces.
- The 120 manual wire joins remain explicit F.Cu/B.Cu connections, with 0.80 mm holes and 1.80 mm lands. These connections exist only after the wires are soldered on both faces.

The transformed diagnostic model has **zero unconnected items**. Its raw report records 17 expected warnings: four USB mechanical-pad type warnings and 13 footprint-library differences caused by deliberately changing component PTH pads into single-face contact models. Those warnings belong to the diagnostic, not the delivered PCB, and are not suppressed or described as a clean DRC. No fabrication geometry is derived from this model.

`audit_unplated.py` regenerates the diagnostic beneath `.audit/`, the raw `unplated-connectivity.rpt`, `construction.json` and `wire-joins.csv`. Run it in a Python environment containing KiCad's `pcbnew` module with `kicad-cli` available. It never rewrites the delivered PCB. The script stops if the diagnostic has missing connections. The board's independent physical review additionally checks front annulus isolation, wire-via body clearance and manufacturer pad geometry.

The raw board still uses KiCad via/PTH constructs for usable editing and netlist comparison. These CAD types do not claim actual plating. Copper continuity after milling and soldering must be checked with a meter. A clean electrical model cannot prove machine isolation, dowel registration, solder access on a physical sample, impedance, EMC or hot plugging.
