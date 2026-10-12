# Rev A validation record

Production PCB/schematic checks were refreshed **2026-10-12** after compact placement, corrected component footprints/power paths, four-layer routing and flush connector alignment. A separate export of the exact staged sources also passed ERC, all-track DRC and schematic parity, without including unrelated local editor changes. The board is a fabrication candidate; no physical board has been built or tested. Firmware results below are **carried forward from 2026-10-11 and were not rerun during this PCB review**.

| Check | Result and date |
| --- | --- |
| Native shared behavior tests, AppleClang 21 | 2026-10-11: 3/3 pass — button/line behavior, contact/hat/axis/encoder behavior, USB descriptor output; carried forward |
| Native device tests, AppleClang 21 | 2026-10-11: 2/2 pass — profile reports/recovery and actual HID report layout; carried forward |
| Firmware cross-build, Arm GNU 15.3.rel1 | 2026-10-11: pass, STM32G0B1CBT6; ELF/BIN/HEX/MAP emitted; carried forward |
| Linked memory | 2026-10-11: 24,812 bytes flash / 6,496 bytes RAM including linker-reserved heap/stack; carried forward |
| KiCad 10.0.6 schematic ERC | 2026-10-12: 0 errors, 0 warnings across two sheets; [report](review/erc.rpt) |
| KiCad PCB DRC / schematic parity | 2026-10-12: 0 violations, 0 unconnected pads, 0 footprint/parity issues; [report](review/drc.rpt) |
| Production layout | 65 × 55 mm, four copper layers; 58% less area than 100 × 85 mm |
| Routing and copper fill | 657 track segments, 103 through vias including 47 GND vias; all four GND zones filled |
| USB L2 reference sampling | 1,350 main-width route samples; none outside filled L2 GND; L2 one filled region; [audit](review/reference-audit.json) |
| External connector alignment | J1 and J2/J3 nominal fronts at east edge x=165.000 mm; manufacturer edge/hole drawings reviewed |
| Exact component identities | 57 populated instances with exact MPN/datasheet/local footprint; four mounting-hole features; [BOM](bom.csv), [footprints](FOOTPRINTS.md) |
| Manufacturing construction / USB geometry | JLC04161H-7628 and 0.2474/0.1501 mm calculated main pair recorded; fabricator approval, coupon and TDR pending |
| Schematic PDF and copper previews | Exported for the revised layout; review package does not replace manufacturing-layer inspection |
| Physical board / Windows, Linux, macOS USB / CAN / power tests | Not run; hardware not built |
| Home-milling layout | 85 × 85 mm body plus removable 5 mm rail; 120 wire joins; ERC 0 errors/warnings, DRC 0 violations/unconnected/parity issues; [assembly and reports](home-milling/README.md) |
| Home unplated-hole construction | Independent audit confirms unchanged manufacturer lands, accessible wire joins and connectivity with ordinary contacts bottom-only / USB shell front-only; [review](home-milling/review/CONSTRUCTION-AUDIT.md), [connectivity model](home-milling/review/UNPLATED-CONNECTIVITY.md) |
| Milling qualification coupon | 45 × 35 mm; DRC 0 violations/unconnected items; includes USB/QFP/SC70 lands and measurement record; [coupon](home-milling/coupon/README.md) |
| Makera Z1 process / physical assembly | Not run; cutter/laminate/registration/slot/hand-solder qualification pending |

The DRC result uses the explicit 0.15 mm pad-to-hole rule for J1's library geometry, the USB class's 0.15 mm clearance, and the recorded minimum power widths. Saved ERC/DRC reports list ignored check categories. No per-item routing exclusions are used; the zero result applies to the enabled rules and does not certify fabricated clearances, assembly fit or electrical behaviour.

Firmware builds used `0xFFFF/0xFFFF` as compile-only placeholders, not allocated USB IDs. libcanard compiles as a dependency but is not exercised as an application Cyphal node. Internal CAN loopback is implemented but has not executed on hardware. Firmware suspend-current handling remains unverified. The PCB changes did not extend the hello-world firmware's protocol scope.

## Reproduce static checks

From this board revision directory, with KiCad 10 on PATH:

```sh
kicad-cli sch erc starter.kicad_sch --exit-code-violations -o review/erc.rpt
kicad-cli pcb drc starter.kicad_pcb --refill-zones --save-board --schematic-parity --all-track-errors --exit-code-violations -o review/drc.rpt
kicad-cli sch export pdf starter.kicad_sch -o review/schematic.pdf
kicad-cli pcb export svg starter.kicad_pcb --mode-single --layers F.Cu,F.Silkscreen,F.Fab,Edge.Cuts --page-size-mode 2 --exclude-drawing-sheet -o review/pcb.svg
```

Expect zero ERC errors/warnings, zero DRC violations/unconnected items and zero schematic/PCB mismatches. Inspect the saved reports and regenerate the manufacturing exports after any change. The [routing record](ROUTING.md) defines current geometry; [FABRICATION.md](FABRICATION.md) defines construction and release checks.

## Remaining fabrication and assembled-board tests

1. Obtain fabricator approval of the specified stackup, final USB geometry, 90 Ω differential ±10% control, hole/slot/mask constraints and coupon/TDR plan. Inspect all exported copper, mask, outline, silkscreen and drill layers before ordering. L2 must remain continuous beneath USB; L3 has local routed branches.
2. Compare actual-size prints against purchased USB/RJ12 connectors, switches and headers. Check contact order, locating pegs, finished holes, cable/latch access, mounting/enclosure clearance and RECOM/C2 height. Review every substitution against its own manufacturer drawing and ratings; dimensional review alone does not prove sample fit.
3. Qualify the specific straight-through twisted-pair CAN cable, endpoint terminator, socket/cable current limits and resistance. Keep separate gateway protection for pass-through power. Test CAN/12 V connector sequencing, local TVS behaviour and hot plugging; local F1 does not protect the whole chain.
4. With a current-limited supply, check 12 V regulation, USB-only operation, both-source handover/removal, USB reverse leakage/backfeed, rail droop/overshoot, reset behaviour, regulator temperatures and shorts/reversed polarity. Verify C4/C5 effective capacitance at operating bias and intended temperature limits, including tolerance/aging. See [ELECTRICAL.md](ELECTRICAL.md).
5. Measure USB power draw before/after configuration, inrush and suspend/resume current, including cable detach while 12 V keeps the MCU powered. Confirm VBUS attachment handling. Implement low-power firmware changes if measurements require them; PCB reverse blocking does not establish USB power compliance.
6. Program/verify over SWD and prove connect-under-reset recovery. Confirm HID/CDC enumeration on Windows/Linux/macOS, both USB orientations, all physical input types, debounce/rearm, eight hat directions/conflicts, potentiometer endpoints/noise, selector conflicts, encoder speed/bounce/overflow, UID persistence and LED commands. Verify queued encoder presses/releases through host stalls, failed transfers and suspend/detach; no old detents may replay.
7. Execute internal CAN FD loopback. External CAN/Cyphal testing requires firmware that enables the transceiver and implements the protocol; then test termination, cable signal margins, load and recovery. Gateway discovery, duplicate suppression, registration and updates remain outside this hello-world validation.
8. For home milling, pass the separate actual-pitch isolation/registration/wire-via coupon before machining the device board. Verify both-face solder access, continuity and insulation with its own assembly procedure; the two-layer construction cannot inherit production impedance qualification.

Record instruments, setup, limits and measurements for each physical test. Clean static checks and an impedance calculator result do not establish measured hardware performance.
