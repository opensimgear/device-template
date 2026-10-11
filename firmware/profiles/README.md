# Representative input profile

The template demonstrates common sim-peripheral inputs through a single joystick and diagnostic CDC interface. Controls are real physical inputs; unused headers do not generate demo movement. The report and physical profile belong to this device; reusable decoding stays in firmware-common. This is an example, not a registered production model or a guarantee to cover every sensor technology.

## Inputs and HID mapping

| Physical control | HID mapping | Wiring |
| --- | --- | --- |
| SW1 momentary button | Button 1 | On-board PA0 switch to ground |
| Analog axis | X, 0–65535 | J5: 1 +3.3 V, 2 wiper, 3 ground; external 10 kΩ linear potentiometer |
| Eight-way hat | Hat: N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7, neutral=8 | J7: 1 ground, 2 N, 3 E, 4 S, 5 W |
| Quadrature encoder | Buttons 3 clockwise / 4 counterclockwise | J8: 1 ground, 2 A, 3 B, 4 push switch |
| Encoder push switch | Button 5 | J8 push contact to ground |
| Maintained toggle | Button 2 closed, released when open | J9: 1 ground, 2 contact |
| Center-open three-position selector | Buttons 6 low / 7 center / 8 high | J10: 1 ground, 2 low, 3 high |

External contacts are passive, normally open contacts to ground, with fitted 10 kΩ pull-ups. Use a hat with four contacts that can close adjacent pairs for diagonals. Use a bare quadrature encoder whose idle detent has both contacts open, with one four-transition electrical cycle per detent; different contact/detent arrangements require changing the decoder. Swap A/B if the desired direction is reversed. The selector can be ON-OFF-ON or a three-position rotary switch whose center throw is left unwired. Do not use powered 5 V/12 V encoder modules on these inputs. Physical controls are external parts to select and purchase separately from the PCB BOM.

Mechanical editions may share this profile. A matrix, extra axes or additional sensing technologies require an explicit profile/board adaptation rather than another mechanical edition identity.

## On-wire report

No report ID; four bytes:

| Byte | Meaning |
| --- | --- |
| 0 | Buttons 1–8 in bits 0–7 |
| 1 | Hat 0–7 or null state 8 in bits 0–3; upper bits zero |
| 2, 3 | X low byte, high byte; unsigned logical range 0–65535 |

The MCU samples the axis at 12-bit resolution and scales raw 0–4095 to the HID range. The 16-bit report does not create 16-bit measurement precision. No saved calibration, inversion or user dead zones are implemented. J5 feeds a 1 kΩ / 100 nF input filter; two 1 MΩ resistors bias an unplugged axis toward center. Allow settling and calibrate endpoint loading on physical hardware. A fresh conversion updates X; after 100 ms without one, X reports 32768 and `axis_ok=0` in serial diagnostics. This is a conversion watchdog, not a reliable cable-disconnection detector. USB recovery also requires a fresh conversion.

## Contact and recovery behavior

Momentary SW1 and encoder push use 10 ms debounce and must first be released for 10 ms after startup, reconnect, resume or failed HID transfer. Already-held contacts cannot create a new press during recovery. The hat must reach debounced neutral before it is armed; opposite contacts or three/four contacts force neutral and disarm it until all contacts are released. Adjacent pairs report diagonals.

Maintained toggle and selector positions become valid after 10 ms and report their physical state, even when already held at startup. An unplugged selector reports center. Simultaneous low/high contacts suppress all three selector buttons and set `selector_conflict`; valid state resumes after debounce when the conflict clears. An unplugged toggle reports released. Their current position has different semantics from a momentary action.

Encoder transitions are sampled once per millisecond. Each full detent queues a direction event; the queue holds 16 waiting events and drops new events when full, incrementing `enc_drop`. Illegal two-bit jumps increment `enc_invalid` and require resynchronization at the open detent. Each direction press lasts at least 25 ms after the host actually completes its press report; its release lasts at least 25 ms after release-report completion before the next event. Host stalls therefore cannot silently consume queued pulses. Nominal sustained output is at most 20 detents/second, usually lower with polling delays; faster bursts can overflow. Timing, bounce, host polling and physical encoder speed still need bench qualification.

USB detach/suspend/recovery discards queued turns and partial detents, releases direction buttons, and requires a fresh detent before new turns. Inactive USB reports neutral buttons/hat/X. There is no CAN input handoff yet. Reports refresh at least every 10 ms when USB is active and ready; changed values can be sent sooner. Game compatibility and loss-free physical sampling are validation gates, not claimed by the host tests.

`src/profile.c` implements this behavior; `src/usb_descriptors.c` defines its descriptor. The `inputs` console command reports button bitmask, X, hat, availability/conflicts and encoder counters. Lite/Pro mechanical changes alone do not require a copied profile.
