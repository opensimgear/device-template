# Production fabrication review package — A draft

This is the **65 × 55 mm four-layer production board**, not the Makera two-layer layout. No order has been placed. The outline centreline gives the stated size; the Gerber job includes the 0.05 mm outline-stroke bounds.

Specify JLCPCB **JLC04161H-7628**, nominal **1.6 mm**, **1 oz outer / 0.5 oz inner** copper, **ENIG**, and **90 Ω differential ±10% controlled impedance** for the main L1 USB pair over L2 GND. Its calculated geometry is **0.2474 mm width / 0.1501 mm gap** with **0.2104 mm L1–L2 dielectric**. Obtain fabricator approval of final etched geometry, laminate/mask construction and a production coupon/TDR report. ImpedanceControlled=true in the job is insufficient as an order specification. See [the full construction and release instructions](../../FABRICATION.md).

The package includes all four copper layers, front paste, both masks/legends, outline and separate PTH/NPTH Excellon drills with maps. Holes/slots are finished dimensions; do not merge or treat PTH as NPTH. USB shell slots are 0.60 mm wide plated slots. RJ12 contacts are 0.89 mm PTH, RJ12 pegs 3.25 mm NPTH and USB locators 0.65 mm NPTH. Confirm manufacturer acceptance of hole, slot, edge and mask constraints.

Before ordering, inspect every manufacturing layer, compare 1:1 prints with purchased parts, approve the exact BOM/substitutions, and freeze an identified release. Current native checks report zero ERC/DRC/parity issues; physical USB, power, CAN, fit and thermal tests remain pending. The current A-draft revision is a review identifier, not a fabrication approval.
