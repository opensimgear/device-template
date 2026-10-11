# Hello-world firmware

C application for `starter_g0b1cb`: STM32G0B1CBT6, LQFP48, 128 KiB flash / 144 KiB RAM. Uses internal HSI/PLL at 64 MHz and HSI48 with USB SOF clock recovery. See the [matching rev-a board](../electronics/boards/starter/rev-a/README.md).

## Build

Install CMake 3.24+, Git, an Arm GNU embedded toolchain with newlib/nano libraries (`arm-none-eabi-gcc`, `objcopy`, `size`) and a build tool. The preset uses Make; Windows users may use Ninja with the explicit configuration below. Put the toolchain on PATH.

From this directory, substitute identifiers you are authorized to use:

```sh
cmake --preset starter -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build --preset starter
```

Alternatively:

```sh
cmake -S . -B build/starter -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake \
  -DOSG_USB_VID=<your-VID> -DOSG_USB_PID=<your-PID>
cmake --build build/starter
```

Outputs are `osg_hello.elf`, `.bin`, `.hex` and `.map` in `build/starter`. Configuration fails if identifiers are missing, zero or outside 16 bits. There are no default allocated USB IDs. CI uses `0xFFFF/0xFFFF` for **compilation only**, never as a recommended identity for flashed/distributed devices.

CMake fetches an exact firmware-common commit; that repository owns all upstream pins. The first build needs network access. For common-code development, add `-DFETCHCONTENT_SOURCE_DIR_FIRMWARE_COMMON=/absolute/path/to/firmware-common`; do not copy common sources into this repository. Native shared-code tests are documented in that repository.

## Flash and recover

The starter PCB is not routed or approved for construction yet. Once a reviewed board is assembled, connect an ST-Link or other compatible SWD probe to J4: 1 target 3.3 V reference, 2 ground, 3 SWDIO, 4 SWCLK, 5 NRST. Power the target from its USB or reviewed 12 V source; J4's 3.3 V pin is a reference, not an invitation to feed another supply into it.

Use STM32CubeProgrammer's SWD interface to program the ELF/HEX at `0x08000000`, verify and reset. Keep NRST available for **connect under reset** recovery. This image has no USB update/DFU command or bootloader. Do not change debug-related option bytes during initial bring-up. The firmware target only describes this custom board, not an unchanged Nucleo board.

## Observe hello world

The PC should enumerate a one-button joystick and a diagnostic serial port. USB HID requires no OpenSimGear app; serial port names and permissions vary by OS. Open the serial port at 115200 baud with DTR asserted (the line rate is informational for USB CDC), send a command ending in LF or CRLF:

| Command | Response / effect |
| --- | --- |
| `hello` | `Hello from OpenSimGear!` |
| `help` | List commands |
| `info` | Model, board, UID, button count and internal-loopback status |
| `led on` / `led off` / `led auto` | Override the physical LED or resume heartbeat |
| `can loopback` | Transmit/receive one 8-byte CAN FD frame inside the MCU; return PASS/FAIL |

Commands are case-sensitive, at most 63 characters. Oversized or NUL-containing input is discarded through the newline and returns an error. Unknown commands return an error. Opening/closing the serial session restores automatic LED behavior.

After startup or USB reconnect/resume, release SW1 for at least 10 ms before pressing. A press held for 10 ms becomes button 1, and releasing for 10 ms releases it. The HID report is one byte: button 1 in bit 0, remaining bits zero. There are no synthetic axes or periodic button presses. Heartbeat changes from 1 s half-period before enumeration to 500 ms after mounting; suspend turns the LED off. Reports are refreshed at least every 10 ms while the host is polling and the endpoint is ready.

## Scope and limits

The transceiver stays in standby, including during internal loopback. A PASS tests the MCU controller, not PCB routing, the transceiver, cable or termination. Linked libcanard is dependency scaffolding; no application Cyphal messages, node-ID allocation, discovery or gateway registration are implemented. The CDC console is diagnostic-only, not the production protocol, SimHub adapter or cockpit configuration API.

VBUS sensing gates USB attachment even when 12 V powers the board. Bench-test this and all USB supply/suspend/inrush behavior; turning off the LED alone does not establish USB suspend-current compliance. The example neither offers production USB compliance nor validates transport ownership/duplicate suppression.
