# OpenSimGear device template

Start a new device with C firmware, electronics and mechanical designs in one repository. This template implements a **STM32G0B1CBT6 hello-world target** and an editable **KiCad 10 starter board**.

## What is implemented

- Representative physical controls: momentary button, analog axis, eight-way hat, rotary encoder with pushbutton, maintained toggle and three-position selector.
- Composite USB HID + CDC serial, unique serial number from the MCU's 96-bit UID.
- Serial `hello`, `info`, `inputs`, LED control and an internal CAN FD loopback diagnostic.
- Shared debouncing, axis scaling, hat decoding, bounded encoder events, line handling, USB descriptor helpers and pinned dependencies in [firmware-common](https://github.com/opensimgear/firmware-common).
- KiCad schematic, project, local symbol/footprint libraries, electrical BOM and a routed two-layer PCB with ground pours and stitching vias.

**Status:** firmware compiles; host behavior tests pass; schematic ERC passes. The routed draft passes copper DRC with zero unconnected items and matches the schematic; manufacturing/impedance and physical validation are still pending. Physical USB, CAN, power and recovery behavior has not been tested. CAN remains in standby: discovery, application Cyphal traffic, gateway handoff, persistent configuration and firmware updates are future integration work.

## Start here

- [Firmware build, flash and console instructions](firmware/README.md)
- [Input examples, HID mapping and control wiring](firmware/profiles/README.md)
- [Starter board and pinout](electronics/boards/starter/rev-a/README.md)
- [Validation record and remaining gates](electronics/boards/starter/rev-a/VALIDATION.md)
- [Schematic PDF](electronics/boards/starter/rev-a/review/schematic.pdf)
- [Front copper preview](electronics/boards/starter/rev-a/review/pcb.svg)

## Create a device

1. Select **Use this template** on GitHub. Replace the device overview and remove unused edition placeholders.
2. Keep common implementation in firmware-common; update its full commit pin deliberately.
3. Define the physical controls and device-specific HID report, identity and capability profile. The diagnostic console is not the ecosystem configuration protocol.
4. Supply USB IDs you are authorized to use before flashing. No public OpenSimGear allocation exists yet; pid.codes is the planned allocation route when eligible.
5. Change the board target and electronics together, adapt the routing and complete the documented checks before fabrication.
6. Add mechanical designs under `hardware/variants/<edition>/<revision>` and maintain a compatibility table mapping mechanical, PCB and firmware revisions.

## Layout

| Directory | Ownership |
| --- | --- |
| `firmware/` | Application, physical control profile, board target and build entry point |
| `electronics/boards/starter/rev-a/` | Editable PCB/schematic, BOM, power/pinout and validation |
| `hardware/variants/lite/` | Future 3D printed mechanical designs |
| `hardware/variants/pro/` | Future laser-cut mechanical designs |

Lite/Pro editions may share firmware and electronics. A material change alone does not require a new controller identity or copied firmware. This is a module template; it does not select the five-port gateway hardware. Template updates do not automatically update repositories created from it.

## Licensing

OpenSimGear license selection remains pending; no license grant is implied for the new authored firmware, electronics or mechanical designs. Included KiCad library data retains its own [license](electronics/boards/starter/rev-a/LICENSE-KICAD.md). Fetched firmware dependencies retain their upstream licenses; see [third-party notices](firmware/THIRD_PARTY.md).
