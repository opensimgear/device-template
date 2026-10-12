# Makera Z1 coupon qualification record

Complete this after physically milling and inspecting the coupon. Blank fields mean **not tested**. CAD/ERC/DRC results do not fill this record.

| Setup | Actual value |
| --- | --- |
| Date / operator / machine | |
| KiCad coupon revision / source hash | |
| CAM/software version | |
| Laminate thickness / copper weight | |
| Isolation tool tip / angle / measured condition | |
| Cutting depth / feed / spindle settings | |
| Surface leveling / fixturing method | |
| Alignment pin diameter / flipping datum | |
| Inspection instrument / resolution | |

Measure surviving copper and cleared channels at several locations. Record the minimum observed width/gap, bridges, burrs and lifted copper. Check continuity along each lane and isolation between different nets.

| Nominal track mm | Nominal gap mm | Minimum measured track | Minimum measured gap | Continuity / isolation / defects |
| --- | --- | --- | --- | --- |
| 0.20 | 0.20 | | | |
| 0.25 | 0.20 | | | |
| 0.30 | 0.20 | | | |
| 0.40 | 0.20 | | | |
| 0.20 | 0.25 | | | |
| 0.25 | 0.25 | | | |
| 0.30 | 0.25 | | | |
| 0.40 | 0.25 | | | |
| 0.20 | 0.30 | | | |
| 0.25 | 0.30 | | | |
| 0.30 | 0.30 | | | |
| 0.40 | 0.30 | | | |

| Physical test | Measurement / result |
| --- | --- |
| USB and LQFP 0.50 mm-pitch lands / 0.20 mm gaps and fanouts | |
| LM66100 0.65 mm-pitch lands / 0.25 mm gaps; all six probe paths | |
| USB 0.65 mm locating holes / 0.60 mm shell slots and actual-part fit | |
| USB shell front-side fillet access and retention on 1.6 mm stock | |
| Front/back registration and minimum remaining wire-via annulus | |
| Four 0.80 mm drilled wire holes; solder both ends / continuity | |
| Adjacent-net isolation after soldering representative parts | |

**Decision:** not tested / repeat coupon / process geometries accepted.

**Measured limits, failures and changes required:**

**Evidence (photos, instrument records):**

An accepted coupon applies only to the tested stock, cutters, fixture and settings. It does not establish full-board assembly access, USB impedance or electrical compliance. Regenerate and inspect CAM/toolpaths for the complete board separately.
