# OpenSimGear device template

Start a new device with C firmware, electronics and mechanical designs in one repository. The **STM32G0B1CBT6 v0.1.0** starter provides **eight analogue inputs, 24 digital lines and optional 16-input I²C expansion boards**. A build-time profile supplies factory defaults; validated runtime configuration selects and calibrates the controls a particular device uses.

## What is implemented

- Configurable buttons, maintained contacts, analogue axes, eight-way hats, rotary encoders and three-position selectors. Unused controls are omitted from the HID descriptor and report.
- `example`, three-command `shifter`, `expanded`, single-button `master_caution` `radio_panel` and CDC-only `output_only` factory profiles; runtime drafts cover every control kind, channel mapping, axis calibration/inversion/deadzone, expansion masks and Unicode controller names. Validation rejects overlapping pins and oversized HID reports.
- One bounded configuration writer, explicit Apply/Save/discard/reset, a separate interrupted-write-safe settings journal, and direct USB descriptor replacement while detached. Cockpit accesses the same runtime core through the matching gateway; the independent JSON device simulation runs this C implementation.
- Optional 3.3 V TCA9555 input boards on I²C, with missing-board diagnostics and recovery that prevents held momentary contacts from generating a new press.
- Composite USB HID + CDC serial, unique serial number from the MCU's 96-bit UID.
- Serial `hello`, `info`, `inputs`, LED control and CAN status diagnostics.
- Device-advertised volatile actions with typed parameters, catalogue revisions and state readback. Cockpit controls the current board's monochrome status LED and the native simulator's RGB LED 1/boolean LED 2 through the same action registry and gateway route, including disabled HID controllers and direct USB ownership.
- Independent mapped-button/encoder transition capture with a 64-record volatile log, 16-record pages, held-state baselines and explicit stream/gap errors. Short debounced presses remain readable while HID is disabled or direct USB owns input. The Master Caution simulator uses the board LED action contract. Radio observations add maintained/selector metadata and independently calibrated axes with availability flags.
- Bounded character-display ports with declared columns, rows and ASCII/UTF-8 scalar-cell encoding, typed text setters and callback readback. The native radio supplies 7-cell frequency, 2-cell preset and 16-cell text ports through production firmware. The starter has no fitted display and advertises only its actual LED.
- Cyphal/CAN device with automatic node-ID selection, standard Heartbeat/GetInfo, profile discovery and input delivery through the STM32 gateway; direct USB ownership and completion-acknowledged encoder events.
- Shared profile validation, HID descriptor generation, debouncing, axis scaling, hat decoding, bounded encoder events, TCA9555 acquisition and line handling in [firmware-common](https://github.com/opensimgear/firmware-common), with pinned dependencies.
- KiCad schematic, exact-part BOM and local libraries, with separate four-layer production and two-layer Makera Z1 layouts. One current circuit is maintained at PCB version 0.1.0; [versions stay fixed until launch](electronics/VERSIONING.md).

**Status on 2026-10-14:** all six factory profiles cross-build and seventeen native suites pass with address/undefined-behavior sanitizers, including independent calibrated axes, bounded display callbacks, panel events, advertised actions, runtime configuration, actual dynamic USB descriptor callbacks, Unicode names, CAN delivery and interrupted settings/boot-journal writes. Cross-firmware tests connect actual acquisition/client code to the [device-gateway](https://github.com/opensimgear/device-gateway) discovery, registry and USB completion logic. Production schematic ERC, PCB DRC and schematic/PCB parity pass with zero violations or missing connections. These are software and CAD checks; no STM32 board has been physically validated. Runtime input configuration and saved calibration are implemented in portable code and the STM32 adapter. Gateway discovery now automatically enables new compatible identities in persistent descriptor order, with logical CAN routes 1–32. Explicitly disabled devices remain configurable without a gateway HID collection; re-enabling appends them. Matching gateway/device helpers are required for routes above ten. Implementation and qualification work is tracked in the [GitHub project](https://github.com/orgs/opensimgear/projects/2).

## Start here

- [Run native devices from JSON](simulation/README.md)
- [Firmware build, flash and console instructions](firmware/README.md)
- [CAN integration, ownership and firmware validation](firmware/CAN_BRINGUP.md)
- [Input examples, HID mapping and control wiring](firmware/profiles/README.md)
- [Starter board and pinout](electronics/boards/starter/0.1.0/README.md)
- [Optional 16-input expansion board wiring](electronics/expansion/tca9555/README.md)
- [Fabrication construction and home-milling plan](electronics/boards/starter/0.1.0/FABRICATION.md)
- [Validation record](electronics/boards/starter/0.1.0/VALIDATION.md)
- [Schematic PDF](electronics/boards/starter/0.1.0/review/schematic.pdf)
- [Front copper preview](electronics/boards/starter/0.1.0/review/pcb.svg)

## Run simulated devices

Start Cockpit normally and run the gateway from the [device-gateway simulation](https://github.com/opensimgear/device-gateway/tree/main/simulation). From this repository, start a separate terminal for each device:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
python3 simulation/run.py --config simulation/configs/button-panel.json
python3 simulation/run.py --config simulation/configs/shifter.json
python3 simulation/run.py --config simulation/configs/master-caution.json
python3 simulation/run.py --config simulation/configs/radio-panel.json
python3 simulation/run.py --config simulation/configs/output-only.json
```

Each JSON declares a unique identity, controller name and input arrays. `output-only.json` and `output-only-board-led.json` declare empty inputs with the same production RGB/boolean or fitted monochrome LED callbacks. They enroll without a HID controller and accept exclusive volatile endpoint leases with independent expiry/fallback. The gateway exposes the devices to Cockpit through its ordinary protocol. The first launch initializes pinned dependencies and builds into this repository’s `.simulation/`; saved configuration survives independent device, gateway and Cockpit restarts. See the [simulation guide](simulation/README.md) for JSON fields, input controls, state isolation and native tests.

Gateway and Cockpit E2E runners clone the components they require under their own `.e2e/` directories and initialize dependencies before building. This repository’s integration workflow clones the gateway runner there and supplies the current device source explicitly. Build commands do not invoke another repository’s E2E runner.

## Create a device

1. Select **Use this template** on GitHub. Replace the device overview and remove unused edition placeholders.
2. Keep common implementation in firmware-common; update its full commit pin deliberately.
3. Copy a firmware profile, assign its physical inputs and HID usages, and select it with `OSG_DEVICE_PROFILE`. The profile supplies factory defaults for USB descriptors, input reports and Cyphal capability announcements. Cockpit can configure the module through the matching gateway, including all input kinds and calibration; the module USB console also accepts `config <child-command>`. Explicit Apply detaches USB before publishing a changed descriptor, and Save persists settings. See the [runtime command contract](https://github.com/opensimgear/protocol/blob/main/serial/runtime-configuration.md).
4. Supply USB IDs you are authorized to use before flashing. No public OpenSimGear allocation is claimed.
5. Change the board target and electronics together, adapt the routing and complete the documented checks before fabrication.
6. Add mechanical designs under `hardware/variants/<edition>/<revision>` and maintain a compatibility table mapping mechanical revisions and PCB versions to compatible firmware targets.

## Layout

| Directory | Ownership |
| --- | --- |
| `simulation/` | Independent native device runtime, JSON examples, build/launch commands and tests |
| `firmware/` | Application, physical control profile, board target and build entry point |
| `electronics/boards/starter/0.1.0/` | Generic PCB/schematic, BOM, pinout and validation |
| `electronics/expansion/tca9555/` | Compatible optional input-board wiring and qualification contract |
| `hardware/variants/lite/` | Reserved Lite mechanical directory |
| `hardware/variants/pro/` | Reserved Pro mechanical directory |

Lite/Pro editions may share firmware and electronics. A material change alone does not require a new controller identity or copied firmware. Mechanical designs remain placeholders. Gateway electronics and firmware live in [device-gateway](https://github.com/opensimgear/device-gateway). Template updates do not automatically update repositories created from it.

## Licensing

No OpenSimGear license grant is implied for the new authored firmware, electronics or mechanical designs. Included KiCad library data retains its own [license](electronics/boards/starter/0.1.0/LICENSE-KICAD.md). Fetched firmware dependencies retain their upstream licenses; see [third-party notices](firmware/THIRD_PARTY.md).

## JLCPCB exports

From this repository root, run `sh tools/bundle-jlcpcb.sh` for the latest numeric board version, or `sh tools/bundle-jlcpcb.sh 0.1.0` for a specific version. Requires KiCad 10 (`kicad-cli`) and Python 3. The script generates a Gerber ZIP plus separate JLCPCB BOM/CPL CSVs in the workspace `.research/jlcpcb/` directory. Use `--output-dir path` to change the destination or `--smd-only` to omit through-hole assembly. See [tool usage](tools/README.md) and the board’s fabrication notes before ordering.
