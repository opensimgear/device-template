# Generic v0.1.0 module firmware

C firmware for `starter_g0b1cb_v0_1_0`: STM32G0B1CBT6 with eight analogue channels, 24 direct digital lines and optional 16-input TCA9555 expansion banks. [v0.1.0 electronics](../electronics/boards/starter/0.1.0/README.md) define the matching pin map. Earlier example circuits remain in Git history and use incompatible wiring; use the current PCB with this target.

## Build factory defaults

Install CMake 3.24+, Git, Make (or Ninja), and an Arm GNU embedded toolchain with newlib/nano libraries. Put `arm-none-eabi-gcc`, `arm-none-eabi-objcopy` and `arm-none-eabi-size` on PATH. From this directory, use USB identifiers you are authorized to use:

```sh
cmake --preset starter -DOSG_DEVICE_PROFILE=example -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build --preset starter
```

`example`, `shifter` and `expanded` are provided. The [profile guide](profiles/README.md) documents wiring, report layouts and adding a profile. Outputs are `osg_hello.elf`, `.bin`, `.hex` and `.map` in this repository’s `.build/starter` directory. Missing, zero or out-of-range USB IDs are rejected. CI uses `0xFFFF/0xFFFF` only for compilation, never for distribution or flashing.

For Ninja instead of Make:

```sh
cmake -S . -B ../.build/starter-ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake \
  -DOSG_DEVICE_PROFILE=shifter -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build ../.build/starter-ninja
```

Both firmware and native tests use the full common-code pin in `cmake/common-revision.cmake`. Common code owns upstream dependency pins. For shared-code development add `-DFETCHCONTENT_SOURCE_DIR_FIRMWARE_COMMON=/absolute/path/to/firmware-common`, then update the commit pin after review. The first normal build needs network access.

## Run a native device from JSON

From the repository root, start one device per terminal:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
python3 simulation/run.py --config simulation/configs/button-panel.json
python3 simulation/run.py --config simulation/configs/shifter.json
```

The [simulation guide](../simulation/README.md) covers device identity, every supported control, input samples and persistent state. The first launch builds this repository’s production runtime and initializes its pinned `firmware-common` dependency under `.simulation/`. Ordinary Cockpit connects to the separately running gateway, which discovers these devices over local CAN transport. No sibling source checkout or workspace setup is required.

JSON is the complete factory definition, including axis inversion and deadzone. The production initializer validates it and restores committed settings when available. Explicit factory reset stages the JSON defaults. Firmware boards retain their existing profile initializer and default axis options.

## Flash and observe

Connect a compatible SWD probe to J4: 1 target 3.3 V reference, 2 ground, 3 SWDIO, 4 SWCLK, 5 NRST. Power from the reviewed USB or 12 V path; the SWD reference is not a separate supply input. Use STM32CubeProgrammer to program and verify the ELF/HEX at `0x08000000`, retaining connect-under-reset recovery. This target is a custom PCB, not an unchanged Nucleo. There is no bootloader or USB flashing command yet. Preserve the independent settings journal at `0x0801E000`–`0x0801EFFF` and the boot-counter journal at `0x0801F000`–`0x0801FFFF` during application updates; if a full erase resets it, power-cycle the entire chain before reconnecting. See [CAN integration](CAN_BRINGUP.md).

The active configuration enumerates as one joystick and a diagnostic USB serial port. Saved settings take precedence over compiled factory defaults. HID contains only enabled controls. USB device revision is `0x0003`; changing the enabled controls changes the report layout and may require refreshing OS/game bindings. Open serial with DTR asserted and send LF/CRLF-terminated commands (115200 is informational for USB CDC):

| Command | Result |
| --- | --- |
| `hello` | `Hello from OpenSimGear!` |
| `help` | Command list |
| `info` | v0.1.0 target, active profile, MCU UID and HID counts |
| `inputs` | Axis-availability mask, hat/selector conflicts, missing/stale expansion-bank mask, blocked command groups and encoder counters |
| `led on`, `led off`, `led auto` | LED override or heartbeat |
| `can status` | Cyphal node ID, logical route, lease, boot session, ownership, RX/TX and recovery counters |
| `config <child-command>` | Bounded runtime configuration command; `OK` or `ERR <reason>` line followed by a JSON configuration snapshot |

The console line buffer holds 391 characters; a configuration child command is limited to 383 characters. Overflow and NUL input are discarded through newline with an error. The LED changes from 1 s to 500 ms half-period after mounting; suspend turns it off. Active USB reports refresh at least every 10 ms when the endpoint is ready. Changed reports can arrive sooner. Encoder pulses advance only after completed host transfers; stale-generation acknowledgements are rejected after USB recovery. TinyUSB retains a submitted interrupt packet across suspend. Its eventual completion cannot acknowledge a new encoder generation, and a canceled/free endpoint cannot leave the software submission flag stuck. A pre-suspend packet may still reach the host before the fresh neutral report on resume; exact host-visible behavior requires physical suspend/resume validation. The template does not claim transparent cancellation of already-submitted USB packets.

## Acquisition and limits

All 24 digital lines receive input pull-ups, all eight ADC channels run in the fixed sequencer, and I²C initializes at startup so a runtime profile can enable any supported input without restarting MCU peripherals. Only declared controls enter reports. A circular double-frame DMA buffer maps each completed half to A0–A7 without waiting for conversions. Only complete fresh frames update enabled axes. USB recovery requires a new frame; a 100 ms conversion watchdog reports each axis's configured neutral. This does not detect an unplugged sensor reliably.

I²C uses PB6/PB7 AF6 with an initial 400 kHz timing target at 64 MHz PCLK. One enabled TCA9555 address is polled per tick in round-robin order. The shared driver configures/validates direction and polarity, then reads both ports; failures immediately invalidate that bank. Each IRQ-based transfer expires after two millisecond tick boundaries, allowing at least 1 ms of wall time even when it starts just before a tick. A 2 ms bank-start budget bounds the whole poll to roughly 3 ms plus software overhead. The blocking HAL memory API's separate 25 ms BUSY wait is avoided. Timeout stops peripheral/IRQ access before callback buffers expire. A later tick performs a fixed nine-clock single-master bus clear and peripheral reinitialization; a stuck line has no unbounded wait. Recovery invalidates all banks and forces fresh configuration. NACKs on missing addresses do not invalidate unrelated healthy banks. Banks without a successful poll for 100 ms become unavailable, including if scanning stops. Timing, recovery pulse widths and all eight banks under mixed faults require bench measurement.

[Expansion wiring](../electronics/expansion/tca9555/README.md) requires unique addresses, suitable contact pull-ups, 3.3 V signals and a capacitance-qualified local bus. Encoders use direct pins; slow I²C polling cannot guarantee capture of quadrature transitions. I²C stays inside a device enclosure.

## Validation and remaining integration

```sh
cmake -S firmware/tests -B .build/tests -DCMAKE_BUILD_TYPE=Release
cmake --build .build/tests
ctest --test-dir .build/tests --output-on-failure
```

Run those commands from the repository root. Ten native tests exercise the runtime core, all supplied factory profiles and their real USB descriptor callbacks, including dynamic field omission and UTF-16 controller names. Shared tests cover limits, invalid assignments, conflicts, stale axes, cross-bank recovery, encoder acknowledgements and expansion scanning. CI also cross-builds each profile. These checks do not establish physical timing or host compatibility.

Build-time profiles supply factory defaults. Runtime configuration owns bounded arrays for buttons, axes, hats, encoders and selectors; axis calibration endpoints, inversion and a rescaled center deadzone; expansion masks; and a UTF-8 controller name. Contact debounce remains fixed at 10 ms. One nonzero writer token claims a draft for 30 seconds, refreshed by successful writer commands. Apply validates the whole draft and requests activation; Save persists the active configuration; Apply & save commits the draft before activation. Discard returns to active settings; reset stages factory defaults. Invalid drafts and failed saves leave active settings unchanged.

Settings use a canonical pointer-free 598-byte record in a dedicated two-page flash journal. On erased storage, initialization saves factory defaults as generation 1 before accepting commands, so even the first user save has a committed fallback. An interrupted initial baseline is retried only when the first page matches a partially written factory image, its commit marker is incomplete and the second page remains erased. Other corrupt nonblank storage fails closed. Saved settings restore after reboot; claims and drafts do not. Direct USB detaches before descriptor replacement, old USB/CAN delivery state is invalidated, acquisition requires fresh input, and USB reconnects after 250 ms. An unread direct-console Apply response is abandoned after four seconds so a stopped reader cannot hold an accepted activation indefinitely; reconnect and read the active generation when an acknowledgement is lost.

The runtime generation is announced separately from the HID schema revision. Every configuration activation renews the gateway lease so old inputs cannot survive same-shape calibration or name changes. Changes to control counts, axis usages or axis neutral values require the gateway's own explicit Apply; its active descriptor remains in place and mismatched slots report neutral until then. Name changes, endpoint calibration, inversion and deadzone changes retain the gateway layout. The [shared command contract](https://github.com/opensimgear/protocol/blob/main/serial/runtime-configuration.md) describes arguments and compact snapshots. SimHub outputs and firmware updates remain future work.

The generic device is a Cyphal/CAN node. The shared libcanard stack selects its transport node ID, publishes standard Heartbeat, serves GetInfo, and carries generated OpenSimGear DSDL transfers for profile discovery and lossless input through the matching STM32 gateway. Gateway USB completion acknowledgements preserve encoder pulse delivery; direct USB takes priority. See [CAN integration and validation](CAN_BRINGUP.md) for ownership, freshness, storage and qualification details. VBUS gates USB attachment when externally powered; assembled supply, suspend, inrush, USB and CAN tests remain required.
