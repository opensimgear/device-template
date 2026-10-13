# Electrical review and bring-up gates

This records the circuit review and selected corrections, not measured hardware validation. The current schematic, PCB and exact-part BOM must remain consistent. See `FOOTPRINTS.md` for drawings/land patterns, `FABRICATION.md` for the two fabrication paths and `VALIDATION.md` for static checks.

## Power path

J2/J3 carry the same linear CAN segment and pass through 12 V power. Each connector has pins 1/6 = BUS_12V, 2/5 = GND, 3 = CAN_H and 4 = CAN_L. Use only a qualified, straight-through, twisted-pair cable with the published wiring; generic flat or reversed telephone cords are not qualified. These are powered CAN ports and must not connect to telephone or network equipment.

The local branch is `BUS_12V → F1 → D1 → BUCK_IN → U4 → BUCK_5V`. F1 is the real **Littelfuse 1206L025/24YR**, 0.25 A hold / 24 V, replacing the nonexistent `/30` selection. D1 is **SS14-E3/61T**. D5 **SMF15A-E3-08** has its cathode on BUCK_IN after D1 and anode on GND. It adds local transient clamping; actual overshoot, surge energy and component temperature still need measurement. It does not establish protection against sustained overvoltage or chain-wide faults. [RECOM R-78E limits](https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf), [TVS ratings](https://www.vishay.com/docs/85881/smf5v0atosmf58a.pdf).

**F1 protects the local load only.** J2/J3 through-power remains connected even if F1 trips or is removed. Cable/contact/copper current and fault protection are supplied and qualified at the gateway; the local fuse does not determine the permitted daisy-chain load.

U6/U7 **LM66100DCKR** replace the two ambiguous Schottky OR branches. Their wiring follows [TI's dual-OR application](https://www.ti.com/lit/ds/symlink/lm66100.pdf):

| Pin/function | U6 bus branch | U7 USB branch |
| --- | --- | --- |
| 1 VIN | BUCK_5V | USB_VBUS |
| 3 active-low CE | USB_VBUS | BUCK_5V |
| 6 VOUT | SYS_5V | SYS_5V |
| 2 GND / 5 unused ST | GND | GND |
| 4 NC | Unconnected | Unconnected |

TI's pin table, page 3, says for ST: “Connect to GND if not required.” The project-specific OR symbol treats VOUT as a passive switched power path and grounded unused ST as passive, retaining power-input checks and SYS_5V's power flag. This scoped ERC model describes this application; it is not a general-purpose status-output symbol or a global relaxation of ERC.

LM66100 input range is 1.5–5.5 V. Reverse blocking limits source backfeeding but is not zero leakage, and its 1.5 A rating does not increase the permitted USB load. Check actual handover, reverse leakage, rails and reset behaviour with both supplies and either supply removed.

## Supplies, USB and controls

C2 is exact **10 µF / 50 V X7R, Murata GRM32ER71H106KA12L**, in **1210**. It adds local converter filtering; no quantified 12 V effective-capacitance or EMC claim has been established. C5 is exact **2.2 µF / 25 V X7R, TDK C2012X7R1E225K125AB**. The [TLV755P requirement](https://www.ti.com/lit/ds/symlink/tlv755p.pdf) is **more than 0.47 µF effective** capacitance at both input and output. Check C5 at the maximum 3.3 V rail and C4 at approximately 5 V, including bias, tolerance, temperature and aging. Typical reference curves aid selection but do not guarantee a combined worst case; see the capacitor discussion in `FOOTPRINTS.md`.

STM32G0B1CBT6 supply/USB pin assignments, 3.3 V operation, internal USB pull-up/matching and USBLC6 channel mapping were reviewed against manufacturer data. Separate 5.1 kΩ CC resistors are retained. VBUS sensing must control USB attachment even while 12 V keeps the MCU powered. U2 pin 5 uses raw USB_VBUS; keep C11 and its ground return close to U2. [MCU](https://www.st.com/resource/en/datasheet/stm32g0b1cb.pdf), [USBLC6](https://www.st.com/resource/en/datasheet/usblc6-2.pdf).

The firmware's 100 mA USB descriptor remains the operating budget; peripheral loads must fit that budget. USB inrush, running current and suspend current are **unverified**. Firmware suspend-current handling is not resolved by this PCB review. No USB-C current negotiation or higher-power output allowance is implied.

The production main USB pair references uninterrupted L2 GND. L3 carries a GND pour, a local BUS_12V feed, CAN_L test spur and v0.1.0 input routes. The bottom USB contact-bridge return corridor is protected; L3 must not be described as a second uninterrupted ground-only plane.

TCAN3413D uses 3.3 V VCC/VIO and a standby pull-up. C1 is the relocated local VCC bypass; the added C13 is a separate 100 nF KEMET C0805C104K3RACTU bypass for VIO, with short local supply connections and adjacent ground vias. R5 was moved to preserve assembly clearances. This follows TI’s supply/layout guidance; transient margin still requires board testing. Its bus-fault/ESD component ratings do not certify whole-board hot-plug immunity. The chain requires endpoint termination only: a gateway endpoint and an end plug at the final module; intermediate modules must not add parallel terminators. External CAN/Cyphal operation still needs firmware and network testing. [TCAN3413](https://www.ti.com/lit/ds/symlink/tcan3413.pdf).

SWD's 3.3 V pin is a **target reference**, not a separate supply input. Do not externally drive it while relying on the LDO for reverse protection. Input headers are for passive 3.3 V contacts/encoder/potentiometer wiring, not 5 V or 12 V signals. Connect control harnesses with power off; cable ESD/hot plugging has not been qualified.

## Generic input and expansion review

v0.1.0 reserves eight ADC pins, 24 pulled-up digital lines and I²C1 PB6/PB7, while retaining USB/CAN/SWD/reset/LED and power sensing. [The pin table](README.md) is the wiring contract. Every axis has its own 1 kΩ / 100 nF filter and 1 MΩ midpoint bias pair; every digital line has its own 10 kΩ pull-up. The old PA0 test switch is removed. Internal oscillators and disabled UCPD dead-battery pull-downs are necessary for the assigned digital pins. The exact non-N STM32G0B1CBT6 is required: the N variant changes PC6/PC7 into supply pins and cannot replace it in this circuit.

The profile chooses sampled ADC/digital resources and generates a matching HID descriptor. Pin conflicts and report-size limits are checked before USB attachment. These software limits do not establish measured input latency, analogue settling or noise. Measure all eight channels, including sparse channel selection and channel-to-channel crosstalk.

J18 powers only a qualified low-power 3.3 V internal I²C harness. R67/R68 are 4.7 kΩ SDA/SCL pull-ups; their ability to meet 400 kHz timing depends on total capacitance and any parallel breakout resistors. TCA9555 input boards must use unique addresses, compatible 3.3 V logic, local bypassing and passive input wiring. Account for input pull-up current, expansion-board consumption and regulator/USB budgets. Do not treat address capacity as proof that eight populated boards meet electrical timing or power limits. See the [expansion contract](../../../expansion/tca9555/README.md).

## First assembled-board checks

1. Inspect polarity, bridges and wire-via joins; measure rail-to-ground and inter-rail resistance before power. Check pin/contact continuity, including both USB orientations and both CAN sockets.
2. Bring up each source separately with a current-limited supply. Check 12 V, BUCK_IN, BUCK_5V, SYS_5V and 3.3 V, temperatures and current. Verify SWD connect-under-reset and reliable startup/reset.
3. Test both sources together and each removal order. Measure USB backfeeding into an unpowered host, inrush, rail droop/overshoot and contact-bounce recovery. Qualify gateway protection and downstream pass-through separately.
4. Test USB enumeration, inputs and reconnect on Windows/Linux/macOS; measure active/suspend current and observe signal quality. Obtain the production coupon/TDR result before claiming controlled impedance.
5. Verify all analogue channels, configured digital controls and TCA9555 inputs against their profile reports. Test missing-board startup, held-contact recovery, I²C NACK/stuck-line recovery and debounce at the worst polling load. Measure I²C rise time, supply budget and actual input/report latency.
6. Test an externally terminated CAN chain at the intended bitrate and qualified cable length; measure power drop and transients. Fault/ESD/thermal tests and long-term connector/mechanical checks remain qualification work.

Record the actual instruments, setup, measured limits and result for each gate. Clean ERC/DRC or successful enumeration cannot substitute for these measurements.
