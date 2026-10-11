# Rev A validation record

Checked 2026-10-11. This is a starter draft, not an assembled or manufacturing-approved board.

| Check | Result |
| --- | --- |
| Native shared behavior tests, AppleClang 21 | 2/2 pass: button/recovery + bounded serial parsing; USB descriptor output |
| Firmware cross-build, Arm GNU 15.3.rel1 | Pass, STM32G0B1CBT6 target; ELF/BIN/HEX/MAP emitted |
| Linked memory | 20,856 bytes flash; 6,272 bytes RAM including linker-reserved heap/stack |
| KiCad 10.0.6 schematic ERC | 0 errors, 0 warnings |
| KiCad PCB geometric DRC | 0 violations with the documented 0.15 mm hole-clearance rule |
| KiCad schematic/PCB parity | 0 issues |
| Missing copper connections | **108 unconnected items — routing not done; overall DRC does not pass** |
| Schematic PDF and placement preview | Exported and inspected |
| Physical board / Windows, Linux, macOS USB / CAN / power tests | Not run; hardware not built |

Firmware builds used `0xFFFF/0xFFFF` as compile-only placeholders, not allocated USB IDs. libcanard compiles as a dependency but is not exercised as an application Cyphal node. Internal CAN loopback is implemented, but cannot be claimed to pass without executing on hardware.

## Reproduce

From this board revision directory, with KiCad 10 on PATH:

```sh
kicad-cli sch erc starter.kicad_sch --exit-code-violations -o review/erc.rpt
kicad-cli pcb drc starter.kicad_pcb --schematic-parity --exit-code-violations -o review/drc.rpt
kicad-cli sch export pdf starter.kicad_sch -o review/schematic.pdf
kicad-cli pcb export svg starter.kicad_pcb --mode-single   --layers F.Cu,F.Silkscreen,F.Fab,Edge.Cuts --page-size-mode 2   --exclude-drawing-sheet -o review/pcb.svg
```

ERC should succeed. DRC must fail on the known unconnected items until routing is completed. The saved reports include default ignored informational checks; no custom routing exclusions were added.

## Gates before construction and use

1. Independently review component symbols/footprints against exact manufacturer drawings, especially USB-C/RJ12 pin numbering, regulator pinout and diode orientation. Finish passive part selection and check availability.
2. Review connector placement, mounting clearance and actual enclosure fit; route power, ground, USB and CAN with a selected stackup and load budget. Complete DRC with **no unconnected items**, retain schematic parity, and review Gerbers before ordering.
3. Review external CAN protection and hot plugging; qualify a specific straight-through twisted-pair cable, socket current, terminator plug and cable resistance. Maintain per-gateway-port protection for the through-power path.
4. With a current-limited supply, check 12 V regulation, USB-only operation, both-source transitions, no damaging USB backfeed, regulator temperatures and shorts/reversed-power behavior.
5. Check USB power draw before/after configuration, inrush, cable detach while 12 V remains, and suspend/resume current. Implement low-power changes if needed to meet USB requirements.
6. Program/verify over SWD; prove connect-under-reset recovery. Confirm HID/CDC enumeration on Windows, Linux and macOS, physical button debouncing/rearm, UID persistence and LED commands.
7. Execute internal CAN FD loopback. External CAN/Cyphal testing requires a separately reviewed firmware change that enables the transceiver and implements the protocol; then test termination, cable signal margins and recovery.

Gateway discovery, duplicate suppression, registration and updates are not part of this hello-world validation.
