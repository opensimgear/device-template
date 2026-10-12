# Home-milling footprint interpretation

Component identities and manufacturer geometry are documented in the [canonical footprint review](../FOOTPRINTS.md) and [BOM](../bom.csv). `HomeMilling.pretty/` retains the same component copper lands and holes; its per-reference identifiers allow KiCad to compare this derived layout with its derived schematics. Rail alignment holes HA1/HA2 are the only extra board mechanics.

Commercial descriptions of plated contacts/shell slots do not apply to the home construction. Long component leads have back-only electrical attachment and isolated front annuli. USB4105 short shell stakes solder on the front and are grounded there. Every layer transition is a wire through a 0.80 mm hole with 1.80 mm lands, soldered on both faces. See [assembly and tooling details](README.md). No conductive barrel is assumed.

The physical pad dimensions, 0.20 mm USB/LQFP gaps and 0.25 mm LM66100 gaps have not been widened or reduced. Process qualification on the [coupon](coupon/README.md) must establish actual milled isolation and solderability. Exact connector slots/drills and front shell access remain sample-fit checks.
