# Footprints and component identities

The starter uses exact component identities in `bom.csv` and hidden `MPN` / `Datasheet` schematic fields. Board-level mechanical features H1–H4 are mounting holes and have no purchasable component MPN. Replacements must be checked against their own package drawing; a matching generic name such as “USB-C”, “4P4C”, “SMA” or “0805” is insufficient on its own.

## Library conventions

- `KiCad.pretty/` contains selected upstream KiCad footprints. See `LICENSE-KICAD.md` for upstream attribution and licensing.
- `OpenSimGear.pretty/` contains footprints authored from manufacturer dimensions for this starter. The `OpenSimGear` library nickname is defined in `fp-lib-table`. Project license selection remains pending; any geometry derived from KiCad library entries retains its upstream library license.
- Each authored footprint records the exact part or package, source URL, drawing revision where available, pad dimensions and orientation in its description. A source drawing is authoritative over distributor preview images or generic catalog metadata.
- Pin numbering must match the symbol-to-net mapping. Connector geometry is defined looking down at the PCB; manufacturer bottom-view drawings must be converted before assigning coordinates. Locating pegs are not electrical pins.
- Use F.Fab for nominal body/assembly information, F.CrtYd for assembly allowance, F.SilkS for markings that remain clear of solderable openings, F.Paste for SMD apertures and F.Mask for solder openings. Do not add solder paste to through-hole contacts.
- Preserve manufacturer lands for fine-pitch ICs and connectors. The requested 0805 passive size is retained for hand assembly; density is obtained through placement and board layout rather than arbitrarily narrowing solder pads.

## Authored footprints used

| Component | Exact MPN | Footprint | Verified source |
| --- | --- | --- | --- |
| J2,J3 | Evercom 5301-4P4C / LCSC C3097715 | `RJ4P4C_Evercom_5301-4P4C_Horizontal` | 5301-440XXX Rev A, 2025-09-23, through [product Documents](https://jlcpcb.com/partdetail/EVERCOM-53014P4C/C3097715) |
| F1 | Littelfuse 1206L025/24YR | `Fuse_Littelfuse_1206L025-24YR` | [1206L datasheet, 2024-02-26](https://www.littelfuse.com/assetdocs/resettable-ptcs-1206l-datasheet?assetguid=2b6a1515-d4ee-4c83-8bd4-152b4901b8f5) |
| U6,U7 | TI LM66100DCKR | `TI_DCK0006A_SC70-6_LM66100` | [LM66100 / DCK0006A 4214835/D, Nov 2024](https://www.ti.com/lit/ds/symlink/lm66100.pdf) |
| D4 | Kingbright AP2012SGC | `LED_Kingbright_AP2012SGC_0805` | [DSAA4050 rev15B](https://www.kingbrightusa.com/images/catalog/SPEC/AP2012SGC.pdf) |
| J4 | Samtec TSW-105-07-L-S | `PinHeader_Samtec_TSW-105-07-L-S_P2.54mm_Vertical` | [TSW recommended PCB layout](https://suddendocs.samtec.com/prints/tsw-xxx-xx-x-x-xx-xxx-footprint.pdf) |
| J5–J13 | Samtec TSW-103-07-L-S | `PinHeader_Samtec_TSW-103-07-L-S_P2.54mm_Vertical` | Same TSW layout; [series print](https://suddendocs.samtec.com/prints/tsw-xxx-xx-xxx-x-xx-xxx-mkt.pdf) |
| J18 | Samtec TSW-104-07-L-S | `PinHeader_Samtec_TSW-104-07-L-S_P2.54mm_Vertical` | Same TSW sources |
| J14–J17 | Samtec TSW-107-07-L-S | `PinHeader_Samtec_TSW-107-07-L-S_P2.54mm_Vertical` | Same TSW sources |

The Evercom origin is contact 1, with component-side coordinates 1=(0,0), 2=(1.27,−2.54), 3=(2.54,0), 4=(3.81,−2.54) mm. All four contacts use 0.90 mm **finished plated holes** and 1.80 mm authored lands. Both locating holes are 3.20 mm NPTH at (−1.91,6.35) and (5.59,6.35), giving 7.50 mm separation. The body spans X −3.71…7.39 and Y −3.80…14.25 mm before rotation; its mouth faces +Y. Nominal body dimensions are 11.10 × 18.05 × 11.45 mm with 3.0 mm contact tails. The drawing specifies 1.5 A per contact and 30 mΩ maximum initial contact resistance; these ratings alone do not approve the chain current or hot plugging.

The independently inspected drawing is archived for analysis as `.research/gateway-datasheets/evercom-5301.pdf`, SHA-256 `870bbd7e703e2c971a4a50ef716cff00093e3872e34a4494434166ad0d645695`. The product Documents link serves that drawing through a temporary URL; no unverified direct PDF URL is substituted. This footprint is not interchangeable with the historical six-contact Amphenol part or a generic “RJ11” footprint. Its contact numbering fixes the powered bus mapping: 1=12 V, 2=CAN_H, 3=CAN_L, 4=GND.

The fuse's lands are 1.00×1.80 mm with 1.80 mm inner gap. The selected fuse is 24 V rated; there is no `1206L025/30` variant in the checked manufacturer table. Its local protection function does not set the chain pass-through current rating.

LM66100 lands are 0.90×0.40 mm at x±1.10, with 0.65 mm pitch along Y. Pin 1 is left/top, then 2/3 down that side; opposite side is 6/5/4 from top to bottom. Pin 1 VIN / 2 GND / 3 CE / 4 NC / 5 ST / 6 VOUT. This reproduces TI's recommended land example; the generic upstream SC70 footprint has different lands.

Samtec header holes are 1.02 mm finished diameter with 1.70 mm lands, preserving 2.54 mm pitch. Pin 1 is the footprint origin and the row extends toward +Y. The 0.635 mm square posts match the selected straight, single-row `-07-L-S` parts.

## Retained package checks

GCT `USB4105-GF-A` retains the upstream USB4105 footprint, checked against the [official drawing](https://gct.co/files/drawings/usb4105.pdf). Both the 0.65 mm NPTH locating holes and 0.60 mm-wide plated shell slots are required. The blank stake option is 0.95 mm; ordering a different stake suffix needs mechanical review. The connector nose and nominal PCB edge are +Y=3.675 mm before rotation.

STM32G0B1CBT6 uses LQFP48, 7×7 mm body, 0.50 mm pitch and no exposed thermal pad. Its upstream lands provide additional solder heel compared with ST's footprint example and match the selected lead geometry. [STM32G0B1 package information](https://www.st.com/resource/en/datasheet/stm32g0b1cb.pdf).

TCAN3413D uses SOIC8; USBLC6-2SC6 uses SOT23-6; TLV75533PDBVR uses DBV/SOT23-5. Their library footprints and netlist pin order were checked against the respective datasheets in the BOM. They do not use exposed-pad package alternatives.

Vishay SS14-E3/61T uses SMA with pad 1 cathode. SMF15A-E3-08 uses SMF/DO-219AB; its stock footprint exactly matches the [Vishay recommended 1.30×1.40 mm lands at 2.90 mm centre spacing](https://www.vishay.com/docs/85881/smf5v0atosmf58a.pdf). Its cathode connects to BUCK_IN after the polarity diode, and its anode connects to ground.

SW2 is Omron B3F-1002-G using the 4.3 mm-height 6 mm tactile-switch footprint. The current design assigns PA0 to A0; it has no dedicated SW1 test switch. The repeated two-terminal schematic pad identities correctly group the two permanently connected leads on each contact side. The standard 6.5×4.5 mm hole pattern is preserved.

C2's exact Murata GRM32ER71H106KA12L is **1210**, not 0805; maximum height is 2.7 mm. Other capacitors and all resistors retain 0805. RECOM R-78E5.0-0.5 uses 2.54 mm pitch SIP pins; allow extra mechanical margin around its 11.6×8.5 mm nominal body because of its published tolerances.

## Copper spacing and fabrication interpretation

Pad dimensions were checked without shrinking vendor lands. The tightest relevant same-row gaps are:

| Feature | Pitch minus pad width | Nominal gap |
| --- | --- | ---: |
| USB4105 signal pads | 0.50−0.30 | 0.20 mm |
| LQFP48 pins | 0.50−0.30 | 0.20 mm |
| TI DCK0006A adjacent pins | 0.65−0.40 | 0.25 mm |
| SOT23-5/-6 adjacent pins | 0.95−0.60 | 0.35 mm |
| SOIC8 adjacent pins | 1.27−0.60 | 0.67 mm |
| Samtec header neighbouring pads | 2.54−1.70 | 0.84 mm |

These are nominal copper checks, not measurements of fabricated isolation. Board DRC must also check routed traces, vias and copper zones. The home layout’s 0.20 mm isolation target is fully consumed at USB/LQFP pad gaps; the commercial GPIOFanout routing class uses a separate 0.15 mm clearance. Solder-mask registration and the manufacturer's capability remain separate fabrication checks. A home-milling process must reproduce these actual pad gaps on a coupon before relying on it.

KiCad hole dimensions specify the required finished geometry; the fabricator selects tooling/plating compensation. Commercial jack contacts, header contacts and USB shell slots must be plated. RJ locating pegs and M3 mounting holes must remain NPTH. Home-milled holes have no plated barrel and require the documented solder-access and wire-via strategy.

## 3D models and substitution checks

The authored 4P4C, fuse, TI, LED and Samtec footprints currently have no verified 3D model. Fabrication geometry is in their copper, drill and mechanical layers. Do not attach the historical six-contact connector model or another manufacturer's connector model merely to make a preview complete. Upstream models are visual aids and do not override the exact MPN drawing; in particular, check C2's larger package/height and the chosen button height when designing an enclosure.

For future models, use millimetres, verify origin/rotation against pin 1 and the cable mouth, and identify the exact MPN/source/revision. Prefer a project-local model with a relative `${KIPRJMOD}` path for portability, recording its redistribution license. If a model is unverified, omit it and document the limitation.

Before populating the first board, compare an actual-size footprint print with the purchased external jacks and check the jack contact order, locating pegs, latch access, connector nose/edge alignment and screw-head clearances. These checks address physical samples; dimensional review and clean DRC alone cannot certify assembly fit, connector durability, USB compliance or hot plugging.

## Capacitor selection and effective capacitance

C5 is **TDK C2012X7R1E225K125AB**, 2.2 µF ±10%, 25 V, X7R, 0805. The [current TDK product page](https://product.tdk.com/en/search/capacitor/ceramic/mlcc/info?part_no=C2012X7R1E225K125AB) identifies this exact part as in production. Its [official characterization sheet dated 2016-01-03](https://product.tdk.com/system/files/dam/doc/product/capacitor/ceramic/mlcc/charasheet/c2012x7r1e225k125ab.pdf) includes DC-bias and temperature curves. A readable [TDK-generated 2013 characterization export](https://datasheet.octopart.com/C2012X7R1E225K125AB-TDK-datasheet-20749776.pdf), archived during this review, gives approximately 2.1–2.2 µF at 3.3 V from its room-temperature curve. This is a typical reference estimate, not a guaranteed lower bound for purchased parts. TDK expressly describes its characteristic graphs as reference data.

The [TI TLV755P datasheet, revision D](https://www.ti.com/lit/ds/symlink/tlv755p.pdf), pin-function note and §7.1.1 require more than 0.47 µF effective capacitance at the regulator pins. C5's 2.2 µF nominal value was selected to give useful bias/tolerance/temperature margin without increasing the output capacitor to a large nominal value. As a screening calculation, nominal capacitance reduced by −10% initial tolerance and −15% X7R temperature change is 1.683 µF before DC-bias and aging effects. Retention exceeding 28% of that value would remain above 0.47 µF. Combining the approximate room-temperature curve with those factors gives about 1.6 µF, comfortably above the minimum for prototype selection; this multiplication does **not** establish a production worst case because the reference curves do not guarantee combined bias, temperature and aging behaviour.

Qualification must verify the actual purchased C5 at the maximum 3.3 V rail voltage and intended board temperature limits, including component tolerance and aging allowance. Obtain bounded manufacturer data or measure effective capacitance with an LCR instrument capable of applying DC bias, then test regulator start-up, load steps and supply handover on populated boards. An unbiased capacitance reading or a nominal “2.2 µF, X7R” label is insufficient. C4, the TLV755 input capacitor, has the same effective-capacitance requirement and must also be checked at its approximately 5 V operating bias.

C2 is **Murata GRM32ER71H106KA12L**, 10 µF ±10%, 50 V, X7R, 1210. The [manufacturer reference sheet](https://search.murata.co.jp/Ceramy/image/img/A01X/G101/ENG/GRM32ER71H106KA12-04A.pdf) establishes its nominal rating and dimensions, but does not establish effective capacitance at 12 V. At this review, the exact base part `GRM32ER71H106KA12` was absent from the public [Murata SimSurfing](https://ds.murata.co.jp/simsurfing/mlcc.html?lcid=en-us) part list, and its public characterization service returned “The specified partnumber not exist”; a nearby part's curve must not be substituted. No numerical 12 V capacitance is therefore claimed for C2. This lookup result alone does not establish the part's lifecycle status.

The [RECOM R-78E datasheet](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf) marks the standard application's external 10 µF input/output capacitors as optional. C2 is additional local input filtering, rather than a documented 10 µF-effective minimum required for this converter's stability. Before using it to meet a quantified ripple, hot-plug or EMC target, measure its effective capacitance at the actual converter input bias and intended temperature range, or obtain exact-part manufacturer bias data. Verify input transients on the board. The optional capacitor alone does not implement RECOM's separate EMC filter suggestions.
