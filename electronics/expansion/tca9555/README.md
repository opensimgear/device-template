# Optional 16-input I²C boards

The v0.1.0 starter supports optional **3.3 V TCA9555 input boards** inside the peripheral enclosure. Each adds 16 passive contact inputs. Firmware profiles select the fitted addresses and contacts; unselected boards are not polled. This is an electrical interface and wiring reference for compatible expansion boards, not a routed expansion PCB or a claim that a third-party breakout has been qualified. The generic starter PCB and firmware provide the host interface and driver.

## Connection to the starter

Use the v0.1.0 J18 pin order, checked against connector pin numbers rather than cable colors:

| J18 pin | Connection |
| --- | --- |
| 1 | +3.3 V to expander VCC |
| 2 | SDA |
| 3 | SCL |
| 4 | GND |

Connect all fitted boards in parallel on these four signals. Set a different address on each board. Use short internal wiring; this connection is separate from the CAN cable between peripherals. Connect expansion boards with power off. The expansion supply shares the starter's 3.3 V regulator and USB power budget; it is not a general lighting or motor power output.

The starter has one 4.7 kΩ pull-up on each bus signal. Disable redundant breakout-board SDA/SCL pull-ups unless their combined resistance has been deliberately checked. These are bus pull-ups; they are separate from the pull-ups on individual contact inputs. Measure rise time and low level with the populated wiring before accepting 400 kHz operation. The initial resistance and timing are prototype choices, not a qualified cable-length or eight-board operating guarantee.

## Compatible board requirements

Use a TCA9555 powered at 3.3 V, with local 100 nF bypass capacitance between VCC and ground. Strap address inputs A2/A1/A0 to defined low/high levels. Provide P00…P07 and P10…P17 with common ground for passive switches. A contact closes its input to ground; the driver's closed-contact bit is active high. The chip supplies weak input pull-ups; external pull-ups may be fitted for the actual wiring environment. Firmware debounces contacts independently.

The implemented driver polls input registers and does not use INT. An unused INT output can remain unconnected; do not tie it directly to either supply. No expansion outputs are enabled by this implementation. Selecting a pin as an output or running another configuration writer against the expander invalidates the input contract.

Check the chosen package's pin numbering. For the DB/PW 24-pin packages, VCC is pin 24, GND 12, SDA 23, SCL 22 and A2/A1/A0 are pins 3/2/21. Other packages differ. The [TI TCA9555 datasheet](https://www.ti.com/lit/ds/symlink/tca9555.pdf) is the authority for the purchased part's pinout and electrical limits. Other I²C expanders are not interchangeable merely because they have 16 pins or the same address range.

## Addresses and firmware contacts

| Board index | A2 A1 A0 | Seven-bit address | Contact indexes |
| --- | --- | --- | --- |
| 0 | 0 0 0 | 0x20 | 24–39 |
| 1 | 0 0 1 | 0x21 | 40–55 |
| 2 | 0 1 0 | 0x22 | 56–71 |
| 3 | 0 1 1 | 0x23 | 72–87 |
| 4 | 1 0 0 | 0x24 | 88–103 |
| 5 | 1 0 1 | 0x25 | 104–119 |
| 6 | 1 1 0 | 0x26 | 120–135 |
| 7 | 1 1 1 | 0x27 | 136–151 |

Within a board, contact 0…7 is P00…P07 and 8…15 is P10…P17. Firmware contact index is `24 + 16 × board_index + contact`. Indexes 0…23 refer to the starter's direct D0…D23 lines. Declare only the required contacts in the [firmware profile](../../../firmware/profiles/README.md). Eight banks plus the direct lines provide 152 physical contact resources, while one profile supports at most 128 logical HID buttons, eight axes and four hats. Encoder directions and selector positions also consume logical buttons; profiles must satisfy both physical and logical limits.

Keep quadrature encoder A/B on direct MCU inputs for this revision. Expansion boards are intended for buttons, maintained switches, hat contacts and selector contacts, with scan timing validated for the intended control. Missing boards, bus faults and recovery must release affected HID states and rearm momentary contacts before accepting another press. A successful register read is not proof of debounce, electrical noise immunity or host compatibility.

## Qualification

Check board part identity, supply and address straps first. Verify register directions, bit order, open/closed readings, simultaneous held contacts, polling intervals and power consumption. Test removal/failure and recovery with a held momentary contact, a selector and a hat. Check that an absent board does not stall USB or leave a pressed HID button. Record bus rise time and the maximum measured poll interval under USB/CAN load. No expansion hardware has been assembled or tested yet.
