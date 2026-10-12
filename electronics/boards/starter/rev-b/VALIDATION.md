# Rev B validation record

Firmware and circuit checks were refreshed **2026-10-12** for the generic input revision. Rev A sources and earlier firmware remain preserved separately. The current revision is a prototype candidate; no physical board or expansion hardware has been built or tested.

## Firmware checks

| Check | Result |
| --- | --- |
| Shared native Release tests, AppleClang 21 | **4/4 pass**, including profile bounds/recovery, expansion polling/freshness and USB descriptors |
| Device native Debug tests with AddressSanitizer/UndefinedBehaviorSanitizer, AppleClang 21 | **10/10 pass**: runtime transactions; all 80 factory-baseline and all 80 first-save flash interruptions; behavior/actual dynamic descriptors for example, shifter and expanded; USB delivery recovery; CAN client/boot journal; shared Cyphal transport |
| Independent source review | Corrected a millisecond-boundary I²C timeout; no remaining code blocker found in ADC DMA, input ownership, fault/rearm or HID packing/delivery |
| Clean pinned dependency build | Shared source fetched at `35e06787fb9696977fe92d20c5ff5afc1e3e1ebf`; no local shared-code override |
| Cross compiler / target | Arm GNU 15.3.1, STM32G0B1CBT6 / `starter_g0b1cb_rev_b`; all three profiles build ELF/BIN/HEX/MAP |

| Profile | Report layout | Flash | RAM including linker-reserved heap/stack |
| --- | --- | ---: | ---: |
| example | 8 buttons, 1 hat, 1 axis; 4 bytes | 65,840 B | 71,032 B |
| shifter | 3 buttons; 1 byte | 65,824 B | 71,032 B |
| expanded | 40 buttons, 8 axes; 21 bytes | 66,004 B | 71,032 B |

Firmware builds use `0xFFFF/0xFFFF` compile-only placeholders, not allocated USB IDs. Factory profile selection is build-time; runtime input mapping, calibration/inversion/deadzone, expansion masks and Unicode names are implemented with validated drafts and separate interrupted-write-safe settings storage. Current Rev B firmware enables the transceiver and implements Cyphal/CAN Heartbeat/GetInfo, profile discovery and gateway input delivery with direct USB ownership. [CAN integration](../../../../firmware/CAN_BRINGUP.md) records software checks; physical CAN/USB handoff and runtime descriptor reconnection remain unqualified; updates and SimHub outputs remain future integration. Native tests cannot measure ADC/I²C/encoder timing or USB host compatibility. A USB packet submitted before suspend can still complete after resume; software queues/generation guards do not claim transparent cancellation of that packet. Record physical suspend/resume behavior before release.

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
| Fine-pitch milling coupon | Preserved [Rev A 45 × 35 mm coupon](../rev-a/home-milling/coupon/README.md); prior static checks pass, physical Makera qualification pending |
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

## Remaining fabrication and assembled-board tests

1. Obtain fabricator approval of the specified stackup, final USB geometry, 90 Ω differential ±10% control, hole/slot/mask constraints and coupon/TDR plan. Inspect all exported copper, mask, outline, silkscreen and drill layers before ordering. L2 must remain continuous beneath USB; its two D12/D14 routes stay outside the protected return area. L3 has power, CAN logic/test and input routes outside the protected duplicate-contact bridge return corridor.
2. Compare actual-size prints against purchased USB/4P4C connectors, switches and headers. Check contact order, locating pegs, finished holes, cable/latch access, mounting/enclosure clearance and RECOM/C2 height. Review every substitution against its own manufacturer drawing and ratings; dimensional review alone does not prove sample fit.
3. Qualify the specific straight-through twisted-pair CAN cable, endpoint terminator, socket/cable current limits and resistance. Keep separate gateway protection for pass-through power. Test CAN/12 V connector sequencing, local TVS behaviour and hot plugging; local F1 does not protect the whole chain.
4. With a current-limited supply, check 12 V regulation, USB-only operation, both-source handover/removal, USB reverse leakage/backfeed, rail droop/overshoot, reset behaviour, regulator temperatures and shorts/reversed polarity. Verify C4/C5 effective capacitance at operating bias and intended temperature limits, including tolerance/aging. See [ELECTRICAL.md](ELECTRICAL.md).
5. Measure USB power draw before/after configuration, inrush and suspend/resume current, including cable detach while 12 V keeps the MCU powered. Confirm VBUS attachment handling. Implement low-power firmware changes if measurements require them; PCB reverse blocking does not establish USB power compliance.
6. Program/verify over SWD and prove connect-under-reset recovery. Confirm HID/CDC enumeration on Windows/Linux/macOS, both USB orientations, all three profiles, eight analogue channels, 24 direct lines, all enabled TCA9555 contacts, missing/held-board recovery, I²C NACK/stuck-line handling, debounce/rearm, eight hat directions/conflicts, potentiometer endpoints/noise, selector conflicts, encoder speed/bounce/overflow, UID persistence and LED commands. Verify queued encoder presses/releases through host stalls, failed transfers and suspend/detach; no old detents may replay.
7. Use the current Rev B Cyphal firmware to verify Heartbeat/GetInfo, automatic node-ID collision recovery, gateway discovery/registration and completed USB input acknowledgements. Test termination, cable signal margins, load, ownership handoff and bus-off recovery. Firmware updates remain outside this validation.
8. For home milling, pass the separate actual-pitch isolation/registration/wire-via coupon before machining the device board. Verify both-face solder access, continuity and insulation with its own assembly procedure; the two-layer construction cannot inherit production impedance qualification.

Record instruments, setup, limits and measurements for each physical test. Clean static checks and an impedance calculator result do not establish measured hardware performance.
