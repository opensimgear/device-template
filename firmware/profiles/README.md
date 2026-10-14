# Factory and runtime device profiles

A device enables only fitted controls by declaring arrays in a profile header. Disabled controls are omitted entirely, including from the USB descriptor. Choose `-DOSG_DEVICE_PROFILE=example`, `shifter` or `expanded`; the filename under this directory selects the profile. Add a header with a `static const osg_profile_t selected_profile` to add a device. These declarations supply factory defaults. The runtime configuration core can stage every supported control kind, validate the complete draft, save it and publish changed USB descriptors after explicit detachment/reconnection. Changes to control counts, axis usages or neutral values require the gateway's own explicit Apply. Name changes, endpoint calibration, inversion and deadzone changes renew its input lease while retaining the gateway layout.

Reusable validation, descriptor construction, debounce, report packing and input decoding are in firmware-common. This repository owns the selected declarations and v0.1.0 physical mapping. Invalid profiles stop before USB attachment; validation rejects overlapping contacts/channels/usages, missing arrays, unsupported counts, invalid ranges, unmapped expansion banks and I²C encoders.

## Profile examples

| Profile | Enabled controls | HID report |
| --- | --- | --- |
| `example` | Button D0, maintained toggle D1, encoder D2/D3 with push D4, NESW hat D5–D8, three-state selector D9/D10, axis A0 | 8 buttons, 1 hat, 1 X axis; 4 bytes |
| `shifter` | D0 up, D1 down, D2 explicit neutral | 3 buttons; 1 byte; no hat or axes |
| `expanded` | 24 direct momentary contacts, 16 contacts at TCA9555 address 0x20, all eight axes | 40 buttons, 8 axes; 21 bytes; no hat |
| `master_caution` | Momentary contact D0 and the existing board status LED | 1 button; 1 byte; no hat or axes |
| `output_only` | No declared inputs; fitted board status LED action | No HID report; CDC-only USB |
| `radio_panel` | Maintained D0/D1, momentary D2, encoders D3/D4 and D5/D6, calibrated axis A0 | 7 buttons, 1 X axis; 3 bytes; no hat |

In `example`, HID buttons are 1 momentary, 2 toggle, 3 encoder push, 4 positive encoder direction, 5 negative direction, 6 selector low, 7 selector centre and 8 selector high. Encoder direction can be reversed by swapping its A/B assignments. A centre-open ON–OFF–ON selector consumes two contacts; low/high together suppress all three selector outputs and set a diagnostic.

The Master Caution native JSON example selects `outputs: "board-status"`, so it
advertises the physical board's `led.status.set_mode` action (`auto`, `on`, `off`)
instead of RGB/boolean endpoints. Its event control is zero-based index 0. This
allows the same simulator binding to target native firmware and the board without
changing an input mapping or output action ID. Electrical validation is deferred.

For `radio_panel`, zero-based logical event controls 0/1 are maintained switches,
2 is momentary, 3/4 are encoder 1 positive/negative and 5/6 encoder 2
positive/negative. Axis 0 uses A0 with factory endpoints 100–3900 and neutral 0.
The matching JSON supplies 7-cell ASCII UHF frequency, 2-cell ASCII preset and
16-cell UTF-8 text output ports; `max_bytes: 63` bounds the UTF-8 port. The actual
starter has no display and advertises no display capabilities. Adding a physical
display port requires its driver and electrical validation, without changing
these portable callback or observation contracts.

## Physical indexes

A0–A7 map to PA0, PA1, PA2, PA3, PA4, PA6, PA7, PB1 (ADC channels 0, 1, 2, 3, 4, 6, 7, 9). Each axis connector supplies +3.3 V, signal and ground; use a suitable potentiometer or 0–3.3 V sensor. The PCB filters/biases each input. Scaling 12-bit samples into 16-bit HID fields does not create 16-bit measurement precision. Profiles supply default endpoint ranges and neutral values. Runtime settings can change and persist these values, invert an axis and apply a rescaled center deadzone in unsigned 16-bit HID units.

| Digital indexes | MCU pins in order |
| --- | --- |
| D0–D5 | PA8, PA9, PA10, PA15, PB2, PB3 |
| D6–D11 | PB4, PB10, PB11, PB12, PB13, PB14 |
| D12–D17 | PB15, PC6, PC7, PC13, PC14, PC15 |
| D18–D23 | PD0, PD1, PD2, PD3, PF0, PF1 |

Digital banks expose six signals and ground. Contacts are active-low passive switches to ground; MCU pull-ups are enabled on all 24 direct inputs so runtime mappings can enable any contact; only declared contacts enter reports. Do not apply 5 V or 12 V. Hats use four contacts NESW; adjacent pairs form diagonals. Encoders require a full four-transition cycle per detent, anchored at both contacts open. Powered encoder modules and other detent arrangements need their own supported interface/decoder.

Expansion input index `24 + 16*bank + pin` selects bank 0–7, pin 0–15, at seven-bit I²C address `0x20 + bank`. Set `expander_mask` for every referenced bank. The expanded sample uses indexes 24–39 and mask `1`. Four expansion boards add 64 contacts; eight add 128 contacts, subject to total report limits and tested bus loading. Each board requires suitable input pull-ups and unique address straps; TCA9555 has weak internal contact pull-ups, with external resistors selected for the actual wiring. See [electronics expansion wiring](../../electronics/expansion/tca9555/README.md).

## Bounds and report construction

The shared validator accepts at most 8 axes, 4 hats, 8 direct encoders, 8 selectors and **128 logical HID buttons total**. A declared button contributes one logical button, an encoder two, and a selector three; those counts share the 128-button budget. Physical direct/expansion contacts cannot overlap. The maximum allowed report is 34 bytes within the 64-byte HID endpoint: 128 button bits, four hat nibbles and eight little-endian 16-bit axes. The available physical pins and contacts still constrain combinations; these maxima are not additive promises.

Wire order is button bits (declared buttons, encoder directions, then selector triples), padding to a byte, hat nibbles with padding to a byte, then axes in declaration order. There is no report ID. Hats use positions 0–7 clockwise from north and null 8; axes declare explicit Generic Desktop usages. Descriptor fields are generated from the same validated layout used to pack reports.

## Recovery behavior

Momentary contacts require a debounced 10 ms release before a new press at startup and USB recovery. Maintained contacts and selectors report fresh physical positions after 10 ms, even if already held. The shifter's three commands share an exclusive group: any simultaneous closures suppress all commands and require all contacts to release before rearming. Its neutral button means an explicit command, not the lever resting position.

Hats arm only after all four contacts are debounced open. Opposite or three/four contacts force null and disarm until full release. A missing expansion bank forces affected controls neutral immediately; recovery requires fresh debounce, and already-held momentary contacts/hats stay disarmed until release. Selectors spanning banks remain neutral while either bank is unavailable. Unused selectors are absent, so they cannot generate a phantom centre button.

Encoders use direct pins with a 1 ms main-loop sampling target. Bounded I²C transactions and bus recovery can lengthen the interval; measure the worst interval for profiles that combine encoders and expansion inputs. Each full detent queues a direction event; 16 waiting events are retained and overflow drops new events with a diagnostic. Illegal two-bit jumps increment another counter and require resynchronization at the open detent. Press and release each last at least 25 ms after the corresponding USB transfer completes; host stalls cannot consume queued events. Recovery discards queued turns and partial detents. Nominal sustained output is at most 20 detents/s before polling delays, and physical speed/bounce still need qualification.

The independent panel event collector retains sampled debounced button/selector
transitions in a 64-record log and emits a press/release pair for each completed
encoder detent. Its zero-based controls use the same logical order as the HID
report. It remains active without HID ownership, uses no InputAck and does not
consume the HID encoder queue. HID recovery does not reset this log; a successful
configuration activation starts a fresh stream. See the [event contract](https://github.com/opensimgear/protocol/blob/main/serial/panel-input-events.md).

Reports are neutral while neither direct USB nor a current CAN assignment is active. Axes require fresh conversion frames and revert to their profile neutral after 100 ms without one. Expansion scanning also has a 100 ms watchdog, with per-bank immediate transaction-failure suppression. Diagnostics are available through `inputs`; CAN handoff uses the same packed report through the matching gateway Cyphal DSDL types, including hats and all 128 logical button bits. Encoder events advance only after the current direct USB transfer or the gateway acknowledges its completed USB transfer.

Zero-input profiles keep a valid nonzero CAN profile revision and publish Status
for routed configuration/actions. They emit no Input report or delivery ACK,
allocate no gateway HID report ID and cannot be enabled as a controller until
inputs are configured. The starter still advertises only its fitted status LED.
