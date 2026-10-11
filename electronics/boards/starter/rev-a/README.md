# STM32G0B1 starter — Rev A draft

Open `starter.kicad_pro` in **KiCad 10**. The schematic and placed PCB are editable and include local libraries; the board is **unrouted and not for fabrication**. No Gerbers are provided. See [validation](VALIDATION.md), [schematic PDF](review/schematic.pdf), [placement SVG](review/pcb.svg) and [BOM](bom.csv).

## Selected components

- ST STM32G0B1CBT6, LQFP48: 128 KiB flash, 144 KiB RAM. VDD/VDDA, VREF+ and VBAT use 3.3 V; this package internally ties VDDIO2/VDDUSB to VDD. [ST datasheet](https://www.st.com/resource/en/datasheet/stm32g0b1cb.pdf).
- GCT USB4105-GF-A USB-C receptacle, external 5.1 kΩ Rd on each CC pin, ST USBLC6-2SC6 protection. USB data pins connect directly; external pull-up is not required.
- TI TCAN3413D 3.3 V CAN FD transceiver, with VIO and VCC at 3.3 V and an external standby pull-up. [TI datasheet](https://www.ti.com/lit/ds/symlink/tcan3413.pdf).
- RECOM R-78E5.0-0.5 12 V-to-5 V switching module followed by TI TLV75533PDBVR 3.3 V LDO. [RECOM datasheet](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf), [TI LDO datasheet](https://www.ti.com/lit/ds/symlink/tlv755p.pdf).
- Two Amphenol 54601-906WPLF RJ12 sockets, used only as a provisional connector candidate.

The CSV identifies IC/connector/fuse candidates. Passive values, voltage ratings and footprints are specified; final vendor part numbers and availability require BOM review. Mounting holes are mechanical features, not components to order. Optional 3D models refer to a normal KiCad model installation and are not vendored.

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

| Contact | Provisional signal |
| --- | --- |
| 1, 6 | BUS_12V |
| 2, 5 | GND |
| 3 | CAN_H |
| 4 | CAN_L |

This is a **bench candidate**, not the final ecosystem cable standard. Use continuity-checked straight-through wiring with CAN_H/L on a qualified twisted pair. A reversed telephone cord swaps CAN polarity; a generic flat telephone cord is not qualified. Never connect these powered ports to telephone or Ethernet equipment. Connector current, cable resistance and signal quality still need tests; RJ45/Cat5e remains the fallback.

Use a linear chain. Gateway termination is at one end; put 120 Ω across CAN_H/L in an end plug at the final module's unused socket. Intermediate modules have no terminator. J6 can accept a temporary 120 Ω test resistor for the final-module position; it must be removed when the module is intermediate. Do not form a ring or bridge independent gateway ports.

## Power paths

12 V is carried on dedicated conductors alongside CAN, never on CAN_H/L. J2/J3 pass BUS_12V straight through; **their chain power is protected at the gateway**. F1 protects only this module's local branch. D1 guards that branch against reversed supply polarity; U4 makes 5 V. D2 and D3 diode-OR regulated bus power and USB VBUS into SYS_5V, then U5 makes 3.3 V. Nominal USB-reachable fitted capacitance is about 3.7 µF, excluding IC/internal capacitance; measure actual inrush.

The diodes block normal reverse power flow, but reverse leakage, simultaneous-source transitions, USB backfeed and fault behavior need measurement. This circuit does not feed external 12 V from USB and does not validate chain hot plugging. J2/J3 expose bus voltage whenever the chain is powered; local fuse removal does not isolate the through-power contacts.

USB descriptors request 100 mA. Use the actual host budget; the prototype only drives a small LED. Do not add high-current outputs to the 3.3 V rail without revisiting USB budgets, regulators and thermal design. U4's nominal 0.5 A rating is not an approved module/chain current budget. Power off before connecting unqualified cables. External CAN surge/ESD protection, live connector sequencing and hot-plug behavior remain review gates before external-bus operation.

## Routing starting point

Two copper layers, 100 × 85 mm outline and four M3 mounting holes. Footprints carry exact exported schematic nets, with no copper tracks or zones. References are on F.Fab to keep placement silkscreen clear. Route and compact the board after component/connector review.

Keep decoupling close to supply pins and a continuous ground return. Route USB as a short pair with protection next to J1; follow the USB/PCB stackup impedance requirement. Keep CAN pass-through short with a short transceiver stub. Size bus conductors, pads and copper for measured chain load and connector limits. The explicit prototype rule sets pad-to-hole clearance to 0.15 mm to accommodate J1's library geometry; a fabricator must approve this and all other constraints. No DRC exclusions hide unfinished routing.

## Libraries and licensing

`KiCad.kicad_sym` and `KiCad.pretty/` contain selected KiCad 10.0.6 library entries under [KiCad's library license](LICENSE-KICAD.md); MCU and inherited symbols are flattened for local editing. `OpenSimGear.kicad_sym` contains the authored TCAN3413D and R-78E symbols. Project license selection is pending, separate from the upstream library license. Both local library tables use project-relative paths.
