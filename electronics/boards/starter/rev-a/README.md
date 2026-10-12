# STM32G0B1 starter — Rev A fabrication candidate

Open `starter.kicad_pro` in **KiCad 10**. The editable production board is **65 × 55 mm with four copper layers**, with USB and both CAN sockets facing the same edge. Its area is 58% smaller than the earlier 100 × 85 mm layout. Exact component footprints, power-source selection and USB construction have been reviewed; fabrication approval and assembled-board measurements remain required. See [fabrication plans](FABRICATION.md), [electrical review](ELECTRICAL.md), [footprint sources](FOOTPRINTS.md), [validation](VALIDATION.md), [schematic PDF](review/schematic.pdf), [front copper](review/pcb.svg), [back copper](review/pcb-back.svg), [routing PDF](review/routing.pdf) and [BOM](bom.csv).

## Selected components

- ST STM32G0B1CBT6, LQFP48: 128 KiB flash, 144 KiB RAM. VDD/VDDA, VREF+ and VBAT use 3.3 V; this package internally ties VDDIO2/VDDUSB to VDD. [ST datasheet](https://www.st.com/resource/en/datasheet/stm32g0b1cb.pdf).
- GCT USB4105-GF-A USB-C receptacle, external 5.1 kΩ Rd on each CC pin, ST USBLC6-2SC6 protection. USB data pins connect directly; external pull-up is not required.
- TI TCAN3413D 3.3 V CAN FD transceiver, with VIO and VCC at 3.3 V, separate local C13/C1 100 nF bypass capacitors and an external standby pull-up. [TI datasheet](https://www.ti.com/lit/ds/symlink/tcan3413.pdf).
- RECOM R-78E5.0-0.5 12 V-to-5 V switching module followed by TI TLV75533PDBVR 3.3 V LDO. [RECOM datasheet](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf), [TI LDO datasheet](https://www.ti.com/lit/ds/symlink/tlv755p.pdf).
- Two Amphenol RJE051660510H RJ12 sockets with manufacturer-derived local footprints and finished contact/locating-hole dimensions.
- Two TI LM66100DCKR ideal-diode devices wired as TI's dual-source OR, plus Vishay SS14-E3/61T local polarity protection, SMF15A-E3-08 local transient protection and Littelfuse 1206L025/24YR local-branch fuse.

The BOM records exact manufacturer parts, datasheets and local footprint references for all **57 populated component instances**. Four mounting holes are mechanical features, giving 61 PCB footprints in total. Resistors and most capacitors remain 0805 for hand assembly; C2 is the specified 1210 Murata part. Availability and any substitutions must be checked before purchase. Authored footprints without a verified 3D model deliberately omit one; retained upstream models use a normal KiCad installation. See [footprint and model conventions](FOOTPRINTS.md).

## Firmware pin agreement

| Signal | MCU port | LQFP48 pin |
| --- | --- | --- |
| Test button, active low | PA0 | 11 |
| Axis, ADC channel 1 | PA1 | 12 |
| Hat N / E / S / W | PA2 / PA3 / PA4 / PA6 | 13 / 14 / 15 / 17 |
| Encoder A / B | PA7 / PA8 | 18 / 28 |
| Encoder push, active low | PA15 | 37 |
| Maintained toggle, active low | PB1 | 20 |
| Selector low / high, active low | PB2 / PB3 | 21 / 42 |
| LED, active high through 1 kΩ | PA5 | 16 |
| VBUS presence, 10 kΩ / 15 kΩ divider | PB0 | 19 |
| USB D− / D+ | PA11 / PA12 pads | 33 / 34 |
| SWDIO / SWCLK | PA13 / PA14 | 35 / 36 |
| CAN standby, active high | PB5 | 44 |
| CAN1 RX / TX, AF3 | PB8 / PB9 | 47 / 48 |
| Reset | PF2-NRST | 10 |

The PA11/PA12 pads may also serve remapped PA9/PA10 functions; leave remapping disabled for USB. PA9/PA10 dead-battery pads 29/32 are unused; firmware disables UCPD dead-battery pull-downs. Clocks use internal oscillators, so no crystal is fitted.

J4 SWD: 1 target 3.3 V reference, 2 GND, 3 SWDIO, 4 SWCLK, 5 NRST. J6 test: 1 CAN_H, 2 CAN_L, 3 GND; no power output on J6.

## External input controls

The `Input examples` child schematic supplies the external control connections. These replace the previous generic J5 GPIO expansion; this unreleased draft revision now uses J5 for the axis. See the [control profile](../../../../firmware/profiles/README.md) for HID mapping and behavior.

| Header | Pin sequence |
| --- | --- |
| J5 axis | 1 +3.3 V, 2 potentiometer wiper, 3 GND |
| J7 hat | 1 GND, 2 N, 3 E, 4 S, 5 W |
| J8 encoder | 1 GND, 2 A, 3 B, 4 push |
| J9 toggle | 1 GND, 2 contact |
| J10 selector | 1 GND, 2 low, 3 high |

Use passive ground-switching contacts, an eight-way hat with adjacent contact pairs, a four-transition-per-detent bare encoder with open contacts at idle, a maintained toggle and a center-open selector. J5 expects a 10 kΩ linear potentiometer between 3.3 V and ground with its wiper on pin 2. External controls are not included in the PCB BOM; select their exact parts and verify contact behavior before assembly. Contacts have 10 kΩ external pull-ups. The axis has a 1 kΩ series resistor, 100 nF filter and 1 MΩ pull-up/down bias to give an unplugged input a defined midpoint. Input settling, analog noise and endpoint calibration need measurement. Never apply 5 V or bus 12 V to these 3.3 V control pins. Connect controls with power off; these headers have no qualified external cable ESD/hot-plug protection.

## CAN connectors and termination

J2 and J3 are electrically parallel pass-through ports on one CAN segment. Socket contacts are numbered by the KiCad footprint/manufacturer drawing, not an assumed cable color sequence.

| Contact | Signal |
| --- | --- |
| 1, 6 | BUS_12V |
| 2, 5 | GND |
| 3 | CAN_H |
| 4 | CAN_L |

This is a **bench candidate**, not the final ecosystem cable standard. Use continuity-checked straight-through wiring with CAN_H/L on a qualified twisted pair. A reversed telephone cord swaps CAN polarity; a generic flat telephone cord is not qualified. Never connect these powered ports to telephone or Ethernet equipment. Connector current, cable resistance and signal quality still need tests; RJ45/Cat5e remains the fallback.

Use a linear chain. Gateway termination is at one end; put 120 Ω across CAN_H/L in an end plug at the final module's unused socket. Intermediate modules have no terminator. J6 can accept a temporary 120 Ω test resistor for the final-module position; it must be removed when the module is intermediate. Do not form a ring or bridge independent gateway ports.

## Power paths

12 V is carried on dedicated conductors alongside CAN, never on CAN_H/L. J2/J3 pass BUS_12V straight through; **their chain power is protected at the gateway**. F1 protects only this module's local branch. D1 guards that branch against reversed supply polarity, D5 clamps local transients after D1, and U4 makes 5 V. U6/U7 LM66100 devices combine regulated bus power and USB VBUS into SYS_5V using cross-connected active-low CE inputs, following TI's dual-OR circuit; U5 then makes 3.3 V. C5 is 2.2 µF / 25 V X7R for regulator margin. Effective capacitance and actual USB inrush require measurement.

The LM66100 devices provide reverse blocking, with reverse leakage, simultaneous-source transitions, USB backfeed and fault behavior still to be measured. This circuit does not feed external 12 V from USB and does not validate chain hot plugging. J2/J3 expose bus voltage whenever the chain is powered; local fuse removal does not isolate the through-power contacts. The local TVS does not establish protection against sustained overvoltage or chain-wide faults.

USB descriptors request 100 mA. Use the actual host budget; the prototype only drives a small LED. Do not add high-current outputs to the 3.3 V rail without revisiting USB budgets, regulators and thermal design. U4's nominal 0.5 A rating is not an approved module/chain current budget. Power off before connecting unqualified cables. CAN cable qualification, live connector sequencing, surge/ESD and hot-plug behavior remain assembled-system test gates. Firmware suspend-current handling is also unverified; the PCB changes do not establish USB power compliance.

## Production layout

The 65 × 55 mm outline retains four M3 mounting holes. J1 is at (161.325, 144.500) mm and J2/J3 at (148.230, 113.000)/(148.230, 130.000) mm, all rotated 90°: their nominal mouths align with the east board edge at x=165.000 mm. The board has **657 track segments and 103 through vias, including 47 GND vias**. L2/In1.Cu is the continuous ground-reference plane; L3/In2.Cu is mostly ground with a local 1 mm BUS_12V feed and a short J6 CAN_L branch. Both outer faces have filled ground pours. USB protection, supply circuitry, MCU decoupling and the analog filter have short local connections. References are on F.Fab to keep silkscreen clear. See [routing constraints and measurements](ROUTING.md).

USB's main pair uses 0.2474 mm width / 0.1501 mm edge gap over the specified JLC04161H-7628 L1–L2 construction, with three front-copper rule areas preserving its noncoplanar corridor. Request 90 Ω differential ±10% fabrication control, approval of final geometry, and a coupon/TDR report. Keep CAN pass-through and transceiver branches short. Size bus conductors, pads and copper for measured chain load and connector limits. The explicit rule sets pad-to-hole clearance to 0.15 mm for J1's library geometry; the fabricator must approve the hole/mask/edge constraints. Native KiCad checks on 2026-10-12 report **ERC 0 errors/warnings, DRC 0 violations, 0 unconnected items and 0 schematic/PCB mismatches**, with no per-item routing exclusions. These results establish static connectivity, not physical qualification.

A separate [home-milling layout](home-milling/README.md) is in progress for an 85 × 85 mm board with a 5 mm alignment rail. It shares this schematic and BOM, but uses a two-layer wire-via construction and must pass the Makera Z1 qualification coupon before machining. It does not inherit the production USB impedance result.

## Libraries and licensing

`KiCad.kicad_sym` and `KiCad.pretty/` contain selected KiCad 10.0.6 library entries under [KiCad's library license](LICENSE-KICAD.md); MCU and inherited symbols are flattened for local editing. `OpenSimGear.kicad_sym` contains authored/application-specific symbols, including the reviewed LM66100 dual-OR model. `OpenSimGear.pretty/` contains manufacturer-derived RJ12, fuse, LED, TI DCK and Samtec header footprints. Project license selection remains pending, separate from the upstream library license. Both library tables use project-relative paths; authored geometry and model conventions are recorded in [FOOTPRINTS.md](FOOTPRINTS.md).
