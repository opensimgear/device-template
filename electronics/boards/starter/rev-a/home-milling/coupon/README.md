# Two-layer milling qualification coupon

This **45 × 35 mm coupon** tests whether the proposed home PCB milling process can reproduce the device-template's smallest features. It is an unpowered geometry and hand-assembly test, **not a working USB circuit, a device PCB, or an impedance test**. No milling toolpaths, machine settings, Gerbers or fabrication order have been created.

Open `milling-coupon.kicad_pro` in KiCad 10, or review [the copper PDF](review/coupon.pdf), [front SVG](review/front.svg) and [back SVG](review/back.svg). The back preview uses top-down coordinates viewed through the board, rather than a mirrored milling setup. Gray `F.Fab` graphics and labels describe the tests; they are not copper and are not intended to be milled. Red/blue preview features are front/back copper; the black holes are NPTH drill features.

## Areas and geometry

Coordinates below are KiCad millimetres. The outline runs from `(100,100)` to `(145,135)`.

| Area | Test |
| --- | --- |
| Upper left, J1 centred `(112,108)`, rotated 180° | Actual stock `Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal` footprint. Four independent 0.20 mm fanout traces on the fine-pitch contacts, ending at separate probe lands. |
| Upper right, U1 centred `(133,109)` | Actual stock `Package_QFP:LQFP-48_7x7mm_P0.5mm` footprint. Four left-edge and six lower-edge pads have 0.20 mm fanouts with width-matched probe lands. |
| Right middle, U2 centred `(140.5,117.5)` | Exact `OpenSimGear:TI_DCK0006A_SC70-6_LM66100` footprint for LM66100DCKR, with **0.65 mm pitch / 0.90 × 0.40 mm rounded lands / 0.25 mm adjacent pad gap**. All six contacts have separate 0.20 mm fanouts and 0.50 × 0.20 mm probe lands, ending at x=138 mm on the left or x=143.4 mm on the right. |
| Bottom left/middle | Three columns for **0.20 / 0.25 / 0.30 mm edge gaps**. Each column has paired 8 mm tracks of **0.20 / 0.25 / 0.30 / 0.40 mm width**, top to bottom. Every width/gap combination is present. Narrow end lands preserve each nominal gap. |
| Bottom right, WV1 and WV2 | Four **0.80 mm drill / 1.80 mm diameter double-sided lands**, centred at `(138,123)`, `(141,123)`, `(138,128)`, `(141,128)`. Two front bridges join each row; separate back protrusions provide probe access. |
| Diagonal corners | Two **2.00 mm NPTH alignment holes** centred at `(102.5,102.5)` and `(142.5,132.5)`, 50 mm apart. These are registration test features; machine fixture design and flipping procedure remain to be established. |

The stock LQFP and fine USB contacts have **0.50 mm pitch, 0.30 mm copper pad width and 0.20 mm nominal interpad gap**. Passing only the 0.25 or 0.30 mm track-gap columns is insufficient: the actual device footprints still require the narrower gap. Tool diameter, tip wear, runout, cutting depth, material flatness and alignment affect the achievable isolated channel; establish these from the test, not a nominal tool label. A wider effective isolation cutter cannot reproduce a narrower channel.

The SC70 area reproduces the actual manufacturer-derived lands used by the device's LM66100 power-source selectors, including their rounded corners and pin numbering. Its 0.25 mm pad gap and six independent probe connections add a separate isolation/solderability test. Use the [TI DCK0006A drawing in the LM66100 datasheet](https://www.ti.com/lit/ds/symlink/lm66100.pdf) when checking an actual part; this area has no power-selection circuit and must remain unpowered.

The unmodified USB footprint also has **0.65 mm locating holes, 0.60 mm-wide elongated shell slots**, and approximately **0.194 mm closest pad-to-locating-hole clearance**. Test these alongside fine-pitch isolation. The custom project records a 0.15 mm hole-to-copper minimum to accommodate this stock geometry; that CAD rule is not a promise of milling accuracy. The separate 0.80 mm wire holes do not replace the connector's smaller mechanical holes or slots.

## No plating or soldermask assumptions

The WV holes are represented by KiCad through-hole pads to display their copper lands on both faces and connect the design model. A **milled board has no plated barrel**: the two faces remain physically separate until a wire or suitable rivet is installed and joined on **both faces**. Solder both sides in the accessible WV area, then test front-to-back continuity and inspect for lifted copper. Do not infer real electrical continuity from the CAD model's automatic PTH connection.

The same absence of plating applies to the USB shell lands. This coupon supplies no circuit, no ground plane and no power. USB fine contacts are independent geometry tests; D+ and D− duplicate contacts are intentionally not joined. Shell pads are unassigned. Optional test-fitting of an actual connector or MCU should assess mechanical alignment and solderability only. There is no electrical reason to attach a cable or power supply.

The home process does not depend on soldermask. Keep exposed adjacent lands clear of solder bridges. On the real device a wire-via layout must also give access to both sides of every manually joined transition; this small coupon cannot establish that accessibility for a denser device layout.

## Record a pass before adapting the device

Use the [qualification record](qualification-record.md) to record the actual machine process and measurements.

1. Measure the surviving copper widths and isolated gaps across all twelve lane pairs; inspect the actual QFP/USB/SC70 pad rows and fanouts under magnification for bridges, torn pads and burrs. Include SC70's 0.25 mm pad gaps and rounded land edges.
2. Confirm continuity along each individual lane and each of the six SC70 pad-to-probe paths, and isolation between different nets. Keep the inspection tool's resolution and observed process variation in the test record.
3. Drill/test the alignment holes, flip using the intended fixture, and measure front/back registration at the four WV lands. Check remaining annulus around every drilled hole rather than assuming the two sides aligned.
4. Install wire/rivet connections in WV1/WV2 and solder both sides. Verify back-probe to front-bridge continuity and that neither row shorts to the other. Check slot and locating-hole fit for the selected USB connector.
5. Record laminate thickness, copper weight, milling/isolation method, effective tool width/depth, fixture/registration procedure, measured feature sizes and which tests failed. Use those measured limits to set the separate home-device board rules.

A successful coupon qualifies the tested process geometries and accessible wire joins. It does not qualify USB impedance, USB electrical compliance, the full device's component footprints, or all assembly access. If the narrow pad row fails, investigate the process or use a different assembly/breakout strategy before milling the compact device board.

## CAD checks and reproduction

KiCad **10.0.6** all-track DRC, refreshed **2026-10-12 with the SC70 area fitted**, reports **0 violations and 0 unconnected items**; see [the saved report](review/drc.rpt). Checks include minimum 0.20 mm track width / copper clearance, 0.50 mm edge clearance and 0.15 mm hole-to-copper clearance. No custom DRC exclusions were added. Ignored check categories remain listed in the report. There is no schematic and no schematic-parity claim. Connectivity checks use KiCad's plated through-hole model and do not verify physical wire joints.

The source `.kicad_pcb` embeds the stock USB/QFP and exact authored SC70 footprints. `OpenSimGear.pretty/` and the project-relative `fp-lib-table` keep the SC70 footprint available for editing; no other working layout is a dependency. `geometry.json` records nominal dimensions and all six SC70 pad/probe coordinates. `make_coupon.py` reproduces the source using the Python bundled with KiCad on this Mac; it reads installed upstream footprints plus the coupon's local SC70 footprint, and adjusts the included standalone project settings. Set `KICAD10_FOOTPRINT_DIR` if your upstream library lives elsewhere. Run the script from any directory; outputs remain beside it. The editable coupon files and previews are sufficient for review without regeneration. Upstream KiCad library data retains [its license](LICENSE-KICAD.md); the authored SC70 footprint follows the device project's pending license selection.
