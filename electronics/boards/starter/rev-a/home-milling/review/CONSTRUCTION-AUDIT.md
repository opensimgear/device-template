# Home-board construction audit

Read-only review of `starter.kicad_pcb`, 2026-10-12. The final construction geometry passes this audit. This accepts the nominal design for process qualification; it does not replace a physical milling coupon or populated-board tests.

## Corrections identified during review

1. **Short USB shell stakes require front-side assembly — resolved in the layout.** GCT USB4105-GF-A shell stakes extend only 0.95 ± 0.15 mm below the seating plane; they do not reach the back of 1.6 mm stock. Bottom-only attachment through unplated shell slots is insufficient. The final layout has a specific front-ground exception for SH pads, positively connected to manual ground joins. Solder all four shell lands from the front for anchoring. Nominally each 1.00 mm-wide land exposes about 0.35 mm beyond the case side; confirm fillet access on the purchased connector. [Manufacturer drawing, B4](https://gct.co/files/drawings/usb4105.pdf).
2. **Front pin isolation needs an actual 0.20 mm moat — resolved.** Initial front copper exclusions extended only 0.05 mm beyond each component through-hole land. Same-net ground fill came within that distance of isolated front ground lands, which native electrical clearance checks do not reject. Final exclusions extend 0.20 mm beyond land bounds, with USB SH the deliberate front-ground exception. The exclusions were rebuilt for every physical pad instance, including Omron's repeated pad numbers, so both leads of each shared contact receive their own exclusion.
3. **Connector mouths now align with the east edge — resolved.** J1 is at (181.325,151), with its nose/recommended PCB edge at x=185. J2/J3 origins are x=168.230, placing their nominal mouths at x=185. The home board now shares the commercial layout's flush-port policy. Actual plug, latch and enclosure fit remains a physical test.

## Geometry verified in the final snapshot

- All **120 transition vias** use 1.80 mm lands and 0.80 mm holes, giving a nominal 0.50 mm radial copper annulus. These are manual wires, soldered to copper on both faces, rather than plated vias.
- The board has **1,603 track segments**, with a minimum instantiated width of 0.20 mm. The preserved USB and LQFP48 land gaps are 0.20 mm; the vendor LM66100 land gap is 0.25 mm. USB shell lands also have a nominal 0.20 mm annulus around their slots. The milling coupon must demonstrate these features on the actual stock and tool settings.
- Component copper land shapes, pitches, relative pad coordinates and hole sizes matched the independently reviewed production footprints exactly; C11's back-face placement preserves its land geometry. Mechanical registration holes are additional home-layout features.
- All **48 physical long-lead pads** retain their own nominal 0.20 mm front copper exclusions. An independent collision check using actual pad/track/filled-zone shapes found no front tracks or filled copper within 0.199 mm of those lands; the 0.001 mm comparison margin avoids treating exact nominal-boundary contact as a failure. The four USB SH pads are intentionally excluded from this isolation rule.
- An independent physical ground graph treated every ordinary component hole as **back-only**, USB SH as **front-only**, and only the manual wire vias as connections between faces. All ground contacts remained in one connected graph; USB SH reached actual manual ground joins. This check did not use a conductive barrel through any component hole. The owner's separate single-face diagnostic model also reports zero unconnected items for all nets.
- Via keepouts cover installed component bounding outlines/courtyards, including the back-face capacitor. The closest reviewed via copper edge was approximately 0.225 mm outside U7's bounding outline at (176.9,157.5). This is nominal geometrical access, not proof that a particular soldering tip fits.
- The board outline was x=95…185 and y=100…185: a **90 × 85 mm blank including the 5 mm registration rail**, and an **85 × 85 mm assembled body** after removing the rail.

## Hole and assembly requirements

| Feature | Retained hole geometry | Home construction |
|---|---|---|
| Manual wire transitions | 0.80 mm drill, 1.80 mm land | Solder both faces before parts obstruct access |
| USB shell | 0.60 × 1.70 / 0.60 × 1.40 mm slots | Unplated; solder front lands, with explicit front ground connection |
| USB locating pegs | 0.65 mm | Mechanical only |
| RJ12 contacts | 0.89 mm | Back solder only; housing obstructs top attachment |
| RJ12 locating pegs | 3.25 mm | Mechanical only |
| Samtec headers | 1.02 mm | Back solder only |
| Omron buttons | 1.10 mm | Back solder only |
| RECOM converter | 1.00 mm | Back solder only |

The RJ12 contact tails are 3.20 mm nominal and RECOM pins 4.10 mm nominal, so they extend through 1.6 mm stock. Omron's reference board pattern explicitly uses 1.6 mm stock. [Amphenol drawing](https://cdn.amphenol-cs.com/media/wysiwyg/files/drawing/p-rje05-166-0x10-h.pdf), [RECOM drawing](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf), [Omron B3F drawing](https://omronfs.omron.com/en_US/ecb/products/pdf/en-b3f.pdf), [Samtec layout](https://suddendocs.samtec.com/prints/tsw-xxx-xx-x-x-xx-xxx-footprint.pdf).

All holes are physically unplated in the home process, even where the KiCad component model uses a through-hole pad. The home README explicitly overrides the copied commercial-board references to plated contact holes and shell slots. The drilling/slotting tool set must support the dimensions above; the 0.80 mm wire-via drill cannot produce a 0.60 mm-wide USB slot. Check purchased parts against a 1:1 print before selecting a different hole or tool diameter.

## Final acceptance and limits

The reviewed source was unchanged during the independent audit:

`SHA256 f6a7428486e841b3b23a99f1f08ba89ce79c24e592b92395c2f1ca75ec7e06f6`

The owner's native report at 2026-10-12 10:54:15 records zero DRC violations, zero unconnected pads and zero footprint errors. This audit read those reports and independently checked the physical geometry; it did not rerun the owner's checker in parallel. Detailed measurements are in `home-construction-final.json`.

No remaining nominal-construction blocker was found. Qualification still requires a coupon made with the actual cutter, stock, fixture and probing procedure; opposite-face registration and every wire join must be inspected. Verify purchased-part fit and USB shell retention. The home layout has no controlled-impedance claim: successful USB enumeration does not establish signal-integrity compliance. Continuity, power-rail/handover and functional USB/CAN tests remain populated-board work.

No source PCB, schematic, footprint or owner files were changed by this audit.
