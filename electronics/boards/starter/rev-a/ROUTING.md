# Rev A routing record

This is a routed, two-layer starter prototype with the existing 100 × 85 mm outline and mounting holes. KiCad 10.0.6 reports zero design-rule violations, zero unconnected items and zero schematic mismatches. This establishes copper connectivity under the recorded rules, not manufacture, signal-integrity or electrical approval. No Gerber/order package is provided.

## Placement and critical paths

USBLC6 protection sits immediately behind J1 in the data path. D+/D− run together on the front copper to the MCU, with a bottom-layer ground-reference corridor that excludes other bottom tracks. The USB-C duplicate contacts use two short rear-copper bridges, each with two 0.5/0.3 mm vias; the protection-to-MCU main path has no signal vias. The protection ground region is explicitly stitched into the shared return.

The main USB tracks are 0.20 mm wide, with 0.30 mm edge gap along the coupled portion. The protection-chip fanout widens that spacing locally. Its main protection-to-MCU routes are approximately 38.1 mm D+ / 39.1 mm D−; the primary connector-to-MCU paths, excluding duplicate-contact branches, are approximately 43.7 mm / 45.1 mm. These are copper lengths, not delay/eye-diagram measurements. They are not a validated 90 Ω geometry: the file retains a nominal 1.6 mm board thickness, and a manufacturer's actual dielectric, copper and coplanar-ground geometry must be used to calculate and approve impedance before manufacturing release. Change width/gap or stackup when that review requires it. Follow [ST's USB hardware guidance](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html) and the chosen fabricator's rules.

CAN H/L use a paired 0.25 mm rear-copper backbone between J2/J3. Their rear-copper center spacing is 1 mm on the central straight run. Short front-copper transceiver branches connect to it through vias; the J3 CAN_L escape crosses CAN_H on the other layer to accommodate the staggered connector pinout. The transceiver is near the backbone: branch lengths to its vias are about 3.5 mm H / 2.5 mm L. J6 is a short test/optional final-module termination stub, about 8.1 mm H / 9.6 mm L. There is no fitted terminator on this intermediate-module board. These geometry choices still need external CAN and cable-margin tests.

The powered-CAN pass-through spine on the front uses 1 mm tracks. Its local feed to F1 may change layers; both transition vias are 0.6/0.3 mm. That branch is protected by the existing 250 mA hold fuse. The regulator, diode OR and capacitors are grouped into a short local circuit. Keep the gateway's separate protection for through power; track width is not an approved connector/cable/chain current rating.

MCU supply capacitors are near the supply pins with 0.20 mm local necks and nearby ground returns. The analog series resistor and filter capacitor are near the MCU's ADC input; the ADC net remains on the front. The remote axis-wiper connection and GPIO controls can use both faces. Firmware pin assignments and external-header numbering are unchanged.

## Copper constraints

| Item | Recorded geometry |
| --- | --- |
| General controls | 0.20 mm tracks, 0.20 mm clearance |
| USB D+/D− | 0.20 mm tracks; 0.15 mm connector bridges/clearance |
| CAN H/L | 0.25 mm tracks, 0.20 mm clearance |
| Local regulator paths | 0.50 mm tracks |
| USB VBUS | 0.30 mm routes with a short 0.25 mm protection-chip neck |
| CAN through-power spine / local unprotected feed | 1.00 mm tracks |
| Normal through vias | 0.60 mm diameter, 0.30 mm drill |
| USB contact/short power escapes | 0.50 mm diameter, 0.30 mm drill |
| Ground pours | Both faces, 0.20 mm clearance; SMT direct and through-hole thermal connections |
| Ground stitching | 81 vias; no intentional isolated ground copper |
| Board edge | 0.50 mm minimum copper clearance; pours inset 0.60 mm |
| Pad-to-hole | Existing 0.15 mm rule for the USB-C library geometry |

The explicit net classes and per-net width rules are in the project and custom rules. A stock hole-clearance rule does not establish a manufacturer's capability. Ground pours use 0.30 mm thermal spokes / 0.25 mm thermal gaps on through-hole pads. Stitching avoids via-in-pad. All final filled ground regions belong to the same connected return.

## Review and reproduction

Critical USB/CAN/power paths were laid out explicitly and locked. Remaining routes were generated locally with [Freerouting 2.5.0](https://github.com/freerouting/freerouting/releases/tag/v2.5.0), then imported into KiCad and checked with filled copper. The final authoritative checks are KiCad ERC, all-track DRC and schematic parity. The working PCB is the editable source; intermediate router/session files are not dependencies.

[Front copper](review/pcb.svg), [back copper](review/pcb-back.svg), and [two-page routing PDF](review/routing.pdf) use the same top-down coordinates. The rear-copper plot is viewed through the board, not mirrored as a bottom-side assembly view. F.Fab overlays identify components; they are not copper. Physical tests and outstanding manufacturer/component checks are listed in [VALIDATION.md](VALIDATION.md).
