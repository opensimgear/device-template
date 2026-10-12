# Generic Rev B module firmware

C firmware for `starter_g0b1cb_rev_b`: STM32G0B1CBT6 with eight analogue channels, 24 direct digital lines and optional 16-input TCA9555 expansion banks. [Rev B electronics](../electronics/boards/starter/rev-b/README.md) define the matching pin map. Rev A uses different wiring; its firmware remains at commit `5a78e46` and the old target name is deliberately rejected.

## Build a device profile

Install CMake 3.24+, Git, Make (or Ninja), and an Arm GNU embedded toolchain with newlib/nano libraries. Put `arm-none-eabi-gcc`, `arm-none-eabi-objcopy` and `arm-none-eabi-size` on PATH. From this directory, use USB identifiers you are authorized to use:

```sh
cmake --preset starter -DOSG_DEVICE_PROFILE=example -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build --preset starter
```

`example`, `shifter` and `expanded` are provided. The [profile guide](profiles/README.md) documents wiring, report layouts and adding a profile. Outputs are `osg_hello.elf`, `.bin`, `.hex` and `.map` in `build/starter`. Missing, zero or out-of-range USB IDs are rejected. CI uses `0xFFFF/0xFFFF` only for compilation, never for distribution or flashing.

For Ninja instead of Make:

```sh
cmake -S . -B build/starter-ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake \
  -DOSG_DEVICE_PROFILE=shifter -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build build/starter-ninja
```

Both firmware and native tests use the full common-code pin in `cmake/common-revision.cmake`. Common code owns upstream dependency pins. For shared-code development add `-DFETCHCONTENT_SOURCE_DIR_FIRMWARE_COMMON=/absolute/path/to/firmware-common`, then update the commit pin after review. The first normal build needs network access.

## Flash and observe

Connect a compatible SWD probe to J4: 1 target 3.3 V reference, 2 ground, 3 SWDIO, 4 SWCLK, 5 NRST. Power from the reviewed USB or 12 V path; the SWD reference is not a separate supply input. Use STM32CubeProgrammer to program and verify the ELF/HEX at `0x08000000`, retaining connect-under-reset recovery. This target is a custom PCB, not an unchanged Nucleo. There is no bootloader or USB flashing command yet.

The selected profile enumerates as one joystick and a diagnostic USB serial port. HID contains only enabled controls. USB device revision is `0x0003`; changing profiles changes the report layout and may require refreshing OS/game bindings. Open serial with DTR asserted and send LF/CRLF-terminated commands (115200 is informational for USB CDC):

| Command | Result |
| --- | --- |
| `hello` | `Hello from OpenSimGear!` |
| `help` | Command list |
| `info` | Rev B target, selected profile, MCU UID and HID counts |
| `inputs` | Axis-availability mask, hat/selector conflicts, missing/stale expansion-bank mask, blocked command groups and encoder counters |
| `led on`, `led off`, `led auto` | LED override or heartbeat |
| `can loopback` | Internal MCU CAN FD loopback PASS/FAIL |

Lines are limited to 63 characters. Overflow and NUL input are discarded through newline with an error. The LED changes from 1 s to 500 ms half-period after mounting; suspend turns it off. Active USB reports refresh at least every 10 ms when the endpoint is ready. Changed reports can arrive sooner. Encoder pulses advance only after completed host transfers; stale-generation acknowledgements are rejected after USB recovery. TinyUSB retains a submitted interrupt packet across suspend. Its eventual completion cannot acknowledge a new encoder generation, and a canceled/free endpoint cannot leave the software submission flag stuck. A pre-suspend packet may still reach the host before the fresh neutral report on resume; exact host-visible behavior requires physical suspend/resume validation. The template does not claim transparent cancellation of already-submitted USB packets.

## Acquisition and limits

Only used digital lines receive input pull-ups; unused lines stay analogue/inactive. The ADC fixed sequencer selects the profile's analogue mask in ascending hardware-channel order. A circular double-frame DMA buffer maps each completed half to A0–A7 without waiting for conversions; a profile with no axes does not start ADC/DMA. Only complete fresh frames update axes. USB recovery requires a new frame; a 100 ms conversion watchdog reports each axis's configured neutral. This does not detect an unplugged sensor reliably.

I²C uses PB6/PB7 AF6 with an initial 400 kHz timing target at 64 MHz PCLK. One enabled TCA9555 address is polled per tick in round-robin order. The shared driver configures/validates direction and polarity, then reads both ports; failures immediately invalidate that bank. Each IRQ-based transfer expires after two millisecond tick boundaries, allowing at least 1 ms of wall time even when it starts just before a tick. A 2 ms bank-start budget bounds the whole poll to roughly 3 ms plus software overhead. The blocking HAL memory API's separate 25 ms BUSY wait is avoided. Timeout stops peripheral/IRQ access before callback buffers expire. A later tick performs a fixed nine-clock single-master bus clear and peripheral reinitialization; a stuck line has no unbounded wait. Recovery invalidates all banks and forces fresh configuration. NACKs on missing addresses do not invalidate unrelated healthy banks. Banks without a successful poll for 100 ms become unavailable, including if scanning stops. Timing, recovery pulse widths and all eight banks under mixed faults require bench measurement.

[Expansion wiring](../electronics/expansion/tca9555/README.md) requires unique addresses, suitable contact pull-ups, 3.3 V signals and a capacitance-qualified local bus. Encoders use direct pins; slow I²C polling cannot guarantee capture of quadrature transitions. I²C stays inside a device enclosure.

## Validation and remaining integration

```sh
cmake -S firmware/tests -B build/tests -DCMAKE_BUILD_TYPE=Release
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

Run those commands from the repository root. Seven native tests exercise all supplied profiles and parse their real descriptors. Shared tests cover limits, invalid assignments, conflicts, stale axes, cross-bank recovery, encoder acknowledgements and expansion scanning. CI also cross-builds each profile. These checks do not establish physical timing or host compatibility.

Profiles are build-time declarations. Runtime cockpit configuration, saved calibration, a production serial protocol, SimHub outputs, firmware updates and transport ownership remain future work. libcanard is linked scaffolding; the CAN transceiver remains in standby and no Cyphal application transfers, discovery or gateway registration are implemented. Internal loopback cannot validate a transceiver, cable or termination. VBUS gates USB attachment when externally powered; assembled supply, suspend, inrush, USB and CAN tests remain required.
