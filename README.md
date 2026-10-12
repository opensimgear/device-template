# OpenSimGear device template

Start a new device with C firmware, electronics and mechanical designs in one repository. The **STM32G0B1CBT6 Rev B** starter provides **eight analogue inputs, 24 digital lines and optional 16-input I²C expansion boards**. A build-time profile selects the controls a particular device uses.

## What is implemented

- Configurable buttons, maintained contacts, analogue axes, eight-way hats, rotary encoders and three-position selectors. Unused controls are omitted from the HID descriptor and report.
- `example`, three-command `shifter` and `expanded` firmware profiles; validation rejects overlapping pins and oversized HID reports.
- Optional 3.3 V TCA9555 input boards on I²C, with missing-board diagnostics and recovery that prevents held momentary contacts from generating a new press.
- Composite USB HID + CDC serial, unique serial number from the MCU's 96-bit UID.
- Serial `hello`, `info`, `inputs`, LED control and an internal CAN FD loopback diagnostic.
- Shared profile validation, HID descriptor generation, debouncing, axis scaling, hat decoding, bounded encoder events, TCA9555 acquisition and line handling in [firmware-common](https://github.com/opensimgear/firmware-common), with pinned dependencies.
- KiCad schematic, exact-part BOM and local libraries, with separate four-layer production and two-layer Makera Z1 layouts. Reviewed Rev A files remain available alongside Rev B.

**Status:** firmware compiles and host behavior tests pass. Production schematic ERC, PCB DRC and schematic/PCB parity pass with zero violations or missing connections. USB uses the specified JLCPCB four-layer construction; fabricator impedance approval and physical validation remain pending. Physical USB, CAN, power and recovery behavior has not been tested. CAN remains in standby: discovery, application Cyphal traffic, gateway handoff, persistent configuration and firmware updates are future integration work.

## Start here

- [Firmware build, flash and console instructions](firmware/README.md)
- [Input examples, HID mapping and control wiring](firmware/profiles/README.md)
- [Starter board and pinout](electronics/boards/starter/rev-b/README.md)
- [Optional 16-input expansion board wiring](electronics/expansion/tca9555/README.md)
- [Fabrication construction and home-milling plan](electronics/boards/starter/rev-b/FABRICATION.md)
- [Validation record and remaining gates](electronics/boards/starter/rev-b/VALIDATION.md)
- [Schematic PDF](electronics/boards/starter/rev-b/review/schematic.pdf)
- [Front copper preview](electronics/boards/starter/rev-b/review/pcb.svg)

## Create a device

1. Select **Use this template** on GitHub. Replace the device overview and remove unused edition placeholders.
2. Keep common implementation in firmware-common; update its full commit pin deliberately.
3. Copy a firmware profile, assign its physical inputs and HID usages, and select it with `OSG_DEVICE_PROFILE`. The profile generates the descriptor and report together. Runtime cockpit configuration and the ecosystem capability protocol remain future work; the diagnostic console is not that protocol.
4. Supply USB IDs you are authorized to use before flashing. No public OpenSimGear allocation exists yet; pid.codes is the planned allocation route when eligible.
5. Change the board target and electronics together, adapt the routing and complete the documented checks before fabrication.
6. Add mechanical designs under `hardware/variants/<edition>/<revision>` and maintain a compatibility table mapping mechanical, PCB and firmware revisions.

## Layout

| Directory | Ownership |
| --- | --- |
| `firmware/` | Application, physical control profile, board target and build entry point |
| `electronics/boards/starter/rev-b/` | Generic PCB/schematic, BOM, pinout and validation |
| `electronics/boards/starter/rev-a/` | Preserved earlier example circuit; use its matching firmware revision |
| `electronics/expansion/tca9555/` | Compatible optional input-board wiring and qualification contract |
| `hardware/variants/lite/` | Future 3D printed mechanical designs |
| `hardware/variants/pro/` | Future laser-cut mechanical designs |

Lite/Pro editions may share firmware and electronics. A material change alone does not require a new controller identity or copied firmware. This is a module template; it does not select the five-port gateway hardware. Template updates do not automatically update repositories created from it.

## Licensing

OpenSimGear license selection remains pending; no license grant is implied for the new authored firmware, electronics or mechanical designs. Included KiCad library data retains its own [license](electronics/boards/starter/rev-b/LICENSE-KICAD.md). Fetched firmware dependencies retain their upstream licenses; see [third-party notices](firmware/THIRD_PARTY.md).
