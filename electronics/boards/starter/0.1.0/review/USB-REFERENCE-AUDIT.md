# Independent production USB reference audit

Reviewed 2026-10-12. **All 11 independent assertions pass.** The final saved PCB SHA-256 is `7ba38c2eb668858974c2e1e376d83eced83b57f8917dde311d581c2b15a0cdac`; the source remained unchanged throughout the audit. [The portable checker](production-usb-reference-review.py) projects the complete copper widths and pad shapes against the native saved ground fill. [The result](production-usb-reference-review.json) records every L2 bridge segment, transition via, spacing and USB check.

The main front USB pair, including its endcaps and all differential connector/component launch pads, has filled L2 ground across its full projected copper width. L2 ground is one connected region. The only L2 signals are the two approved local exceptions outside the protected USB region:

| Signal | Approved transition sites (mm) | Actual routed length | Minimum planar gap to all USB copper |
| --- | --- | --- | --- |
| D14 | (128.0, 126.2) → (128.05, 118.9) | 8.3148 mm | 1.175001 mm |
| D12 | (125.4571429, 129.0071429) → (117.65, 115.85) | 17.0445 mm | 4.269176 mm |

The spacing includes the transition-via copper and all USB tracks, pads and connector copper projections. Both exception routes use 0.15 mm tracks.

The duplicate USB contact connections include two short 0.15 mm back-face bridges:

| Signal | Transition-via centres (mm) | Bridge length | Fully referenced copper area beyond the launch antipads |
| --- | --- | --- | --- |
| USB_DP | (155.3, 143.45) → (155.3, 145.05) | 1.6000 mm | 0.104982 mm² |
| USB_DM | (156.2, 143.95) → (156.4, 145.55) | 1.61245 mm | 0.107032 mm² |

The four signal vias have 0.50 mm L3 copper diameters and 0.20 mm plane clearance, giving nominal 0.45 mm antipad radii. Their ends necessarily have short reference discontinuities. Native saved fill represents these circles with a maximum 0.005 mm polygon error; its boundary is approximately 0.45439 mm from each via centre. The audit excludes only the actual saved plane-hole portion inside a separate 0.455 mm bounded radius around each signal via. It never excludes an entire merged hole. All bridge copper outside these local exclusions has filled L3 ground beneath it, with a nonempty referenced centre span. No additional uncovered copper remains.

This confirms the continuous main USB reference and the documented short duplicate-contact launch discontinuities. It does not measure impedance, certify the branch geometry or replace the specified manufacturer stackup and populated USB validation.

```sh
python3 production-usb-reference-review.py /path/to/starter.kicad_pcb /path/to/production-usb-reference-review.json
```

The checker supports no-argument execution when placed in the board's `review/` directory. It never edits or refills the source PCB.
