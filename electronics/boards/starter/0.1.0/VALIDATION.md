# v0.1.0 validation record

Firmware and circuit checks were refreshed **2026-10-12** for the generic input revision. Earlier circuits and matching firmware remain in Git history. The current version is a prototype candidate; no physical board or expansion hardware has been built or tested.

## Firmware checks

| Check | Result |
| --- | --- |
| Shared native Release tests, AppleClang 21 | **4/4 pass**, including profile bounds/recovery, expansion polling/freshness and USB descriptors |
| Device native Debug tests with AddressSanitizer/UndefinedBehaviorSanitizer, AppleClang 21 | **10/10 pass**: runtime transactions; all 80 factory-baseline and all 80 first-save flash interruptions; behavior/actual dynamic descriptors for example, shifter and expanded; USB delivery recovery; CAN client/boot journal; shared Cyphal transport |
| Independent source review | Corrected a millisecond-boundary I²C timeout; no remaining code blocker found in ADC DMA, input ownership, fault/rearm or HID packing/delivery |
| Clean pinned dependency build | Shared source fetched at `35e06787fb9696977fe92d20c5ff5afc1e3e1ebf`; no local shared-code override |
| Cross compiler / target | Arm GNU 15.3.1, STM32G0B1CBT6 / `starter_g0b1cb_v0_1_0`; all three profiles build ELF/BIN/HEX/MAP |

| Profile | Report layout | Flash | RAM including linker-reserved heap/stack |
| --- | --- | ---: | ---: |
| example | 8 buttons, 1 hat, 1 axis; 4 bytes | 65,840 B | 71,032 B |
| shifter | 3 buttons; 1 byte | 65,824 B | 71,032 B |
| expanded | 40 buttons, 8 axes; 21 bytes | 66,004 B | 71,032 B |

Firmware builds use `0xFFFF/0xFFFF` compile-only placeholders, not allocated USB IDs. Factory profile selection is build-time; runtime input mapping, calibration/inversion/deadzone, expansion masks and Unicode names are implemented with validated drafts and separate interrupted-write-safe settings storage. Current v0.1.0 firmware enables the transceiver and implements Cyphal/CAN Heartbeat/GetInfo, profile discovery and gateway input delivery with direct USB ownership. [CAN integration](../../../../firmware/CAN_BRINGUP.md) records software checks; physical CAN/USB handoff and runtime descriptor reconnection remain unqualified. Native tests cannot measure ADC/I²C/encoder timing or USB host compatibility. A USB packet submitted before suspend can still complete after resume; software queues/generation guards do not claim transparent cancellation of that packet. Record physical suspend/resume behavior before release.

## Circuit and layout checks

| Check | Result |
| --- | --- |
| Independent generic circuit/firmware agreement | **179 checks pass**: MCU package pins, eight axis filter/bias networks, 24 digital pull-ups, I²C power/pull-ups, header geometry and both four-contact CAN pinouts |
| Evercom 4P4C footprint geometry | **23 checks pass**, including contact numbering, 0.90 mm contact holes and both 3.20 mm locating holes |
| Circuit purchasing BOM | **107 populated instances** with exact MPNs; four separate mounting-hole features, 111 PCB footprints |
| Production schematic ERC | **0 errors and 0 warnings** |
| Production all-track DRC / parity | **0 violations, 0 unconnected items, 0 schematic/PCB mismatches** |
| Production layout and routing | **85 × 75 mm, four copper layers**, 3,757 segments, 247 vias including 64 GND vias |
| USB ground-reference audit | **11/11 assertions pass**: one connected L2 GND region; full-width main pair and launch pads covered; only D12/D14 on L2, at least 4.269/1.175 mm from USB copper. Back-bridge central widths have L3 ground outside individually bounded via antipads. |
| Production exports | Schematic/routing PDFs, front/back SVGs, all four copper Gerbers plus mask/paste/legend/edge, separate PTH/NPTH drills/maps and Gerber job file |
| Home-milling native checks and unplated assembly model | **0 ERC errors/warnings, 0 all-track DRC violations, 0 unconnected items, 0 parity mismatches**; electrical-face model also has **0 missing connections** |
| Home independent construction audit | **18/18 assertions pass**: all **70 electrical nets** physically connected; 111 canonical geometries retained; 79 bottom-only contacts plus four front-only shell contacts have **83 individual isolation moats**; minimum wire-land/body gap **0.2034 mm** |
| Home construction and wire joins | **105 × 125 mm body plus 5 mm registration rail**, 1,701 segments and **224 soldered wire joins** (1.80 mm lands / 0.80 mm holes); C6–C9 and C11 on the back |
| Fine-pitch milling coupon | Preserved [v0.1.0 45 × 35 mm coupon](home-milling/coupon/README.md); prior static checks pass, physical Makera qualification pending |
| Physical USB / CAN / power / ADC / encoders / I²C / milling | **Not run**; hardware not built |

The exact **non-N STM32G0B1CBT6** is required; the N variant reassigns PC6/PC7 to supplies and cannot substitute in this pin map. Header lands retain the manufacturer dimensions. Static zero-violation results apply to the documented enabled rules; they do not establish measured electrical performance or fabricate the prototype.

## Reproduce static checks

From this board revision directory, with KiCad 10 on PATH:

```sh
kicad-cli sch erc starter.kicad_sch --exit-code-violations -o review/erc.rpt
kicad-cli pcb drc starter.kicad_pcb --refill-zones --save-board --schematic-parity --all-track-errors --exit-code-violations -o review/drc.rpt
kicad-cli sch export pdf starter.kicad_sch -o review/schematic.pdf
kicad-cli pcb export svg starter.kicad_pcb --mode-single --layers F.Cu,F.Silkscreen,F.Fab,Edge.Cuts --page-size-mode 2 --exclude-drawing-sheet -o review/pcb.svg
```

Expect zero ERC errors/warnings, zero DRC violations/unconnected items and zero schematic/PCB mismatches. Inspect the saved reports and regenerate the manufacturing exports after any change. For the independent polygon and unplated construction checks, use the Python bundled with KiCad (with `pcbnew` available), from this directory:

```sh
python3 review/production-usb-reference-review.py starter.kicad_pcb review/production-usb-reference-review.json
python3 review/home-construction-review.py home-milling/starter.kicad_pcb . review/home-construction-review.json
```

These scripts read the saved filled board without modifying it. The home check uses `netlist.xml`, the canonical libraries and the home `source-footprints.json` to verify every electrical net with the actual solder-face assumptions. Regenerate the canonical XML netlist after schematic edits. The [routing record](ROUTING.md) defines current geometry; [FABRICATION.md](FABRICATION.md) defines construction and release checks.

Physical qualification is tracked in the [GitHub project](https://github.com/orgs/opensimgear/projects/2). No physical results are recorded above. Fabrication and electrical documents retain the construction and bench-use procedures.

## Version consolidation checks — 2026-10-13

The single current design is PCB version **0.1.0**, frozen until launch under [PCB versioning](../../../VERSIONING.md). CAD title blocks, silkscreen, firmware target names, documentation and review/manufacturing exports now identify this version. Source comparison against the preceding design confirms copper and circuit connectivity are unchanged. Native KiCad 10.0.6 ERC and all-track DRC with schematic parity pass with zero violations and unconnected items. All three matching device Arm profiles (`example`, `shifter`, `expanded`) build, and all 10 native tests pass. The production and home-milling CAD variants both pass ERC/DRC. Cross-builds use compile-only USB ID placeholders; physical qualification remains pending.
