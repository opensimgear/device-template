# Generic STM32G0B1 starter — v0.1.0

Open `starter.kicad_pro` in **KiCad 10**. The current design provides **eight analogue inputs, 24 digital lines and I²C for optional 16-input expansion boards**. Firmware profiles assign those resources to the controls each device needs; unused controls are omitted from its USB HID descriptor. The matching board target is `starter_g0b1cb_v0_1_0`. Current firmware supports direct USB and Cyphal/CAN discovery/input delivery through [device-gateway](https://github.com/opensimgear/device-gateway); [CAN integration](../../../../firmware/CAN_BRINGUP.md) records software validation.

The four-layer production and separate two-layer Makera Z1 layouts retain USB and both CAN sockets on the same edge. Both use the four-contact powered-CAN connector standard. See [fabrication](FABRICATION.md), [electrical review](ELECTRICAL.md), [footprints](FOOTPRINTS.md), [validation](VALIDATION.md), [schematic PDF](review/schematic.pdf), [front copper](review/pcb.svg), [back copper](review/pcb-back.svg), [routing PDF](review/routing.pdf) and [BOM](bom.csv). No physical board has been fabricated or tested.

This is the single current design. Earlier example circuits remain in Git history and have incompatible pin maps and six-contact CAN wiring. Follow [PCB versioning](../../../VERSIONING.md): 0.1.0 stays fixed until launch. Mechanical Lite/Pro variants must explicitly document PCB and firmware compatibility.

## Selected components

- ST STM32G0B1CBT6, LQFP48: 128 KiB flash, 144 KiB RAM. VDD/VDDA, VREF+ and VBAT use 3.3 V; this exact non-N package internally ties VDDIO2/VDDUSB to VDD. Do not substitute STM32G0B1CBT6N: that variant uses PC6/PC7 pads as supplies and is incompatible with D13/D14 wiring. [ST datasheet](https://www.st.com/resource/en/datasheet/stm32g0b1cb.pdf).
- GCT USB4105-GF-A USB-C receptacle, external 5.1 kΩ Rd on each CC pin, ST USBLC6-2SC6 protection. USB data pins connect directly; external pull-up is not required.
- TI TCAN3413D 3.3 V CAN FD transceiver, with VIO and VCC at 3.3 V, separate local C13/C1 100 nF bypass capacitors and an external standby pull-up. [TI datasheet](https://www.ti.com/lit/ds/symlink/tcan3413.pdf).
- RECOM R-78E5.0-0.5 12 V-to-5 V switching module followed by TI TLV75533PDBVR 3.3 V LDO. [RECOM datasheet](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf), [TI LDO datasheet](https://www.ti.com/lit/ds/symlink/tlv755p.pdf).
- Two Evercom 5301-4P4C sockets (LCSC C3097715), with manufacturer-derived local footprints and finished contact/locating-hole dimensions. [Product and drawing under Documents](https://jlcpcb.com/partdetail/EVERCOM-53014P4C/C3097715).
- Two TI LM66100DCKR ideal-diode devices wired as TI's dual-source OR, plus Vishay SS14-E3/61T local polarity protection, SMF15A-E3-08 local transient protection and Littelfuse 1206L025/24YR local-branch fuse.

The BOM records exact manufacturer parts, datasheets and local footprint references for all **107 populated component instances**. Four mounting holes are mechanical features, giving 111 PCB footprints in total. Resistors and most capacitors remain 0805 for hand assembly; C2 is the specified 1210 Murata part. Availability and any substitutions must be checked before purchase. Authored footprints without a verified 3D model deliberately omit one; retained upstream models use a normal KiCad installation. See [footprint and model conventions](FOOTPRINTS.md).

## Input resources and wiring

All input headers are **3.3 V only**. Use passive ground-switching contacts or appropriate 0–3.3 V sensors. Never apply 5 V or bus 12 V to them. Connect the internal control harness with power off; these headers have no qualified external-cable ESD/hot-plug protection. External controls and expansion boards are not included in the base-board BOM.

### Eight analogue inputs

Every axis header has **pin 1 +3.3 V, pin 2 signal, pin 3 GND**. A 10 kΩ linear potentiometer uses pin 1/3 as its ends and pin 2 as its wiper.

| Input | Header | MCU port | LQFP48 pin | ADC channel |
| --- | --- | --- | ---: | ---: |
| A0 | J5 | PA0 | 11 | 0 |
| A1 | J7 | PA1 | 12 | 1 |
| A2 | J8 | PA2 | 13 | 2 |
| A3 | J9 | PA3 | 14 | 3 |
| A4 | J10 | PA4 | 15 | 4 |
| A5 | J11 | PA6 | 17 | 6 |
| A6 | J12 | PA7 | 18 | 7 |
| A7 | J13 | PB1 | 20 | 9 |

Each signal has a 1 kΩ series resistor, 100 nF filter and 1 MΩ pull-up/down bias for a defined unplugged midpoint. Firmware continuously scans all eight channels and reports only enabled axes, scaling the 12-bit ADC readings to the profile's 16-bit HID range; this does not make the ADC a 16-bit measurement device. Settling, noise, sensor loading and calibration need assembled-board measurement. The profile's disconnected value and the physical midpoint bias serve different purposes.

### Twenty-four digital lines

Each line has a 10 kΩ pull-up to 3.3 V. Close a contact between its signal and the header's ground. These are input resources, not general-purpose high-current outputs.

| Header | Pins 1–6, in order | Pin 7 |
| --- | --- | --- |
| J14 | D0, D1, D2, D3, D4, D5 | GND |
| J15 | D6, D7, D8, D9, D10, D11 | GND |
| J16 | D12, D13, D14, D15, D16, D17 | GND |
| J17 | D18, D19, D20, D21, D22, D23 | GND |

| Line | Port / package pin | Line | Port / package pin |
| --- | --- | --- | --- |
| D0 | PA8 / 28 | D12 | PB15 / 27 |
| D1 | PA9 / 29 | D13 | PC6 / 30 |
| D2 | PA10 / 32 | D14 | PC7 / 31 |
| D3 | PA15 / 37 | D15 | PC13 / 1 |
| D4 | PB2 / 21 | D16 | PC14 / 2 |
| D5 | PB3 / 42 | D17 | PC15 / 3 |
| D6 | PB4 / 43 | D18 | PD0 / 38 |
| D7 | PB10 / 22 | D19 | PD1 / 39 |
| D8 | PB11 / 23 | D20 | PD2 / 40 |
| D9 | PB12 / 24 | D21 | PD3 / 41 |
| D10 | PB13 / 25 | D22 | PF0 / 8 |
| D11 | PB14 / 26 | D23 | PF1 / 9 |

A button or maintained contact consumes one line; an encoder consumes two, plus one for its pushbutton; a hat uses four; a center-open three-position selector uses two. These counts share the same 24-line budget. Keep encoder A/B on direct MCU inputs; polled I²C contacts can miss rapid quadrature transitions.

### Optional 16-input boards

**J18: pin 1 +3.3 V, pin 2 SDA, pin 3 SCL, pin 4 GND.** I²C1 uses PB7/SDA (package pin 46) and PB6/SCL (45). R67/R68 provide one 4.7 kΩ pull-up on each line. Keep links short within the enclosure and disable duplicate breakout pull-ups unless the combined pull-up/capacitance has been checked. The initial 400 kHz setting requires rise-time and load qualification on the actual harness.

Use 3.3 V **TI TCA9555** boards with unique addresses `0x20`–`0x27`. Each provides 16 additional contact inputs. Firmware profiles explicitly enable banks; the implementation supports up to eight addressed banks, subject to bus/power qualification and its logical HID limits. A missing or failed board releases its controls and reports a diagnostic; recovered momentary contacts require release before a new press. See the [expansion-board wiring contract](../../../expansion/tca9555/README.md) and [profile definitions](../../../../firmware/profiles/README.md). This is support for compatible expansion boards, not a routed expansion PCB or automatic device detection.

### Reserved functions

| Signal | MCU port | LQFP48 pin |
| --- | --- | --- |
| Status LED, active high through 1 kΩ | PA5 | 16 |
| VBUS sense, 10 kΩ / 15 kΩ divider | PB0 | 19 |
| USB D− / D+ | PA11 / PA12 pads | 33 / 34 |
| SWDIO / SWCLK | PA13 / PA14 | 35 / 36 |
| CAN standby, active high | PB5 | 44 |
| CAN1 RX / TX, AF3 | PB8 / PB9 | 47 / 48 |
| Reset switch SW2 | PF2-NRST | 10 |

Internal oscillators leave PF0/PF1 and PC14/PC15 available for digital contacts. Firmware disables UCPD dead-battery pull-downs on PA9/PA10 and leaves USB pad remapping disabled. SW1 is removed: its previous PA0 pin is now A0. J4 SWD remains 1 target 3.3 V reference, 2 GND, 3 SWDIO, 4 SWCLK, 5 NRST. J6 test remains 1 CAN_H, 2 CAN_L, 3 GND, without a power output.

## CAN connectors and termination

J2 and J3 are electrically parallel pass-through ports on one CAN segment. Socket contacts are numbered by the KiCad footprint/manufacturer drawing, not an assumed cable color sequence.

| Contact | Signal |
| --- | --- |
| 1 | BUS_12V |
| 2 | CAN_H |
| 3 | CAN_L |
| 4 | GND |

These are **4P4C** sockets, not six-position RJ11/RJ12 sockets. This powered pinout is incompatible with the historical six-contact circuit. Use continuity-checked straight-through wiring with CAN_H/L on a qualified twisted pair. Reversed handset cords swap the supply and ground as well as CAN polarity; generic flat or coiled cords are not qualified. Never connect these powered ports to telephone or Ethernet equipment. Connector current, cable resistance, signal quality and hot plugging still require tests.

Use one linear bus with exactly two endpoint terminations, each 120 Ω across CAN_H/L. An end plug in the last module's unused socket connects **contacts 2 and 3 only**. Intermediate modules have no terminator. If the gateway is an endpoint, enable its termination; if it sits between two chains on the same bus, terminate both outer module ends and disable the gateway termination. J6 can accept a temporary 120 Ω test resistor for an endpoint position; remove it at an intermediate position. Do not form a ring or bridge independent CAN buses.

## Power paths

12 V is carried on dedicated conductors alongside CAN, never on CAN_H/L. J2/J3 pass BUS_12V straight through; **their chain power is protected at the gateway**. F1 protects only this module's local branch. D1 guards that branch against reversed supply polarity, D5 clamps local transients after D1, and U4 makes 5 V. U6/U7 LM66100 devices combine regulated bus power and USB VBUS into SYS_5V using cross-connected active-low CE inputs, following TI's dual-OR circuit; U5 then makes 3.3 V. C5 is 2.2 µF / 25 V X7R for regulator margin. Effective capacitance and actual USB inrush require measurement.

The LM66100 devices provide reverse blocking, with reverse leakage, simultaneous-source transitions, USB backfeed and fault behavior still to be measured. This circuit does not feed external 12 V from USB and does not validate chain hot plugging. J2/J3 expose bus voltage whenever the chain is powered; local fuse removal does not isolate the through-power contacts. The local TVS does not establish protection against sustained overvoltage or chain-wide faults.

USB descriptors request 100 mA. Use the actual host budget; the prototype only drives a small LED. Do not add high-current outputs to the 3.3 V rail without revisiting USB budgets, regulators and thermal design. U4's nominal 0.5 A rating is not an approved module/chain current budget. Power off before connecting unqualified cables. CAN cable qualification, live connector sequencing, surge/ESD and hot-plug behavior remain assembled-system test gates. Firmware suspend-current handling is also unverified; the PCB changes do not establish USB power compliance.

## Layout and validation

The production layout uses the named JLCPCB four-layer construction in [FABRICATION.md](FABRICATION.md). Its main USB pair retains 0.2474 mm width / 0.1501 mm edge gap over a continuous L2 ground reference, with a preserved front-copper corridor. Request 90 Ω differential ±10% fabrication control, final geometry approval and a coupon/TDR report. The extra analogue front ends and headers increase the board area while preserving 0805 passives and manufacturer lands for hand assembly.

The separate [home-milling layout](home-milling/README.md) uses two layers, 1.6 mm laminate and accessible soldered-wire layer joins. It shares the circuit and purchasing BOM, with footprint-only schematic variants for unplated-hole construction. Its USB route does not inherit the production impedance result. Qualify the Makera Z1 process with the [v0.1.0 fine-pitch coupon](home-milling/coupon/README.md) before machining this board.

See [ROUTING.md](ROUTING.md) and [VALIDATION.md](VALIDATION.md) for the final dimensions, native checks, routing metrics and physical test gates. Static connectivity checks do not establish physical USB, CAN, power, input or expansion-board performance.

## Libraries and licensing

`KiCad.kicad_sym` and `KiCad.pretty/` contain selected KiCad 10.0.6 library entries under [KiCad's library license](LICENSE-KICAD.md); MCU and inherited symbols are flattened for local editing. `OpenSimGear.kicad_sym` contains authored/application-specific symbols, including the reviewed LM66100 dual-OR model. `OpenSimGear.pretty/` contains manufacturer-derived 4P4C, fuse, LED, TI DCK and Samtec header footprints. The upstream library license does not grant a license for separately authored project material. Both library tables use project-relative paths; authored geometry and model conventions are recorded in [FOOTPRINTS.md](FOOTPRINTS.md).
