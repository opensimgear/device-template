# Independent home PCB construction audit

Reviewed 2026-10-12. **All 18 independent assertions pass.** The saved source reviewed is `starter.kicad_pcb`, SHA-256 `226ac51b054deb06d5f2783f80ed65319555dda89bbfc872e878c582b2b86aec`. The source remained unchanged throughout the audit.

The [portable checker](home-construction-review.py) reads the PCB, the canonical production `netlist.xml` and manufacturer libraries, and the home `source-footprints.json` and `HomeMilling.pretty`. It never edits or refills the fabrication source. [The complete result](home-construction-review.json) records the source-contract hashes, all assertions and measurements.

| Construction check | Result |
| --- | --- |
| Canonical components, pin assignments and manufacturer land/drill geometry | All 111 component references match; both embedded pads and home libraries preserve their production geometry |
| Physical connectivity | All 70 nets connect using the actual soldered copper faces and explicit wire joins; all ground contacts form one group |
| Wire joins | 224 joins, each with 1.80 mm lands and 0.80 mm drill; all are accessible beyond both-face component bounds |
| Smallest wire-land to component-bound gap | 0.2034 mm |
| Long-lead component contacts | 79 contacts connect on the bottom face; each has its own front copper moat |
| USB shell contacts | Exactly four short shell stakes connect on the front face; each has its own back copper moat and reaches a ground wire join |
| Copper isolation moats | All 83 physical contact instances satisfy the nominal 0.20 mm target; no opposite-face track or filled copper reaches the isolated annulus |
| Copper stack | Two copper faces, nominal 35 µm / 1.53 mm core / 35 µm, total 1.60 mm; no solder-mask thickness is assumed |
| Tracks | 1,701 segments; minimum nominal width 0.20 mm, with wider routes elsewhere |
| Board and connector arrangement | 105 × 125 mm body plus 5 mm registration rail, giving a 110 × 125 mm blank; USB and both 4P4C mouths align at x = 185 mm |

C6–C9 and C11 are soldered on the back face. The connectivity graph uses their actual copper layer set. Ordinary component holes never bridge faces in this audit. A manual wire via bridges faces only because assembly requires a wire soldered on both sides. The paired leads of SW2 and the shared USB shell are joined internally by their physical components; the separate USB A/B signal contacts still require PCB routing.

The exact selected CAN connector is Evercom 5301-4P4C (C3097715), with 0.90 mm contact drills, 3.20 mm holes for both plastic pegs, and the verified manufacturer pin positions. The canonical netlist enforces pin 1 = 12 V bus power, pin 2 = CAN_H, pin 3 = CAN_L, pin 4 = GND at both sockets.

Run with Python from a KiCad installation that provides `pcbnew`. Explicit paths work before and after packaging:

```sh
python3 home-construction-review.py /path/to/home-milling/starter.kicad_pcb /path/to/rev-b /path/to/home-construction-review.json
```

When placed in `home-milling/review/` beneath the canonical board directory, the checker also supports running without arguments from any working directory. The canonical directory must contain `netlist.xml`, `OpenSimGear.pretty` and `KiCad.pretty`; the home directory must contain `source-footprints.json` and `HomeMilling.pretty`.

The independent result agrees with the [native unplated connectivity check](UNPLATED-CONNECTIVITY.md), which reports zero missing connections after modelling the documented solder faces. These checks establish nominal CAD construction and connectivity. Milling registration, actual cutter isolation, component fit, wire soldering, power and USB/CAN operation still require the documented physical qualification coupon and populated prototype checks. No impedance measurement or CNC toolpath qualification is claimed.
