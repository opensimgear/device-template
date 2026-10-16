# Device tools

## Development status and disclaimer

This project is heavily under development and is not ready for use. Use it entirely at your own risk. To the fullest extent permitted by applicable law, the authors and contributors disclaim all liability for any damage, loss, or other issues arising from its use.

## JLCPCB fabrication and assembly files

```sh
sh tools/bundle-jlcpcb.sh                  # highest numeric board version
sh tools/bundle-jlcpcb.sh 0.1.0            # explicit version
sh tools/bundle-jlcpcb.sh --smd-only       # SMD assembly only
sh tools/bundle-jlcpcb.sh 0.1.0 --output-dir ../.research/jlcpcb-custom
```

Requires Python 3 and KiCad 10 on PATH; use `--kicad-cli /path/to/kicad-cli` for another installation. Calls work from any directory. Default output is workspace `.research/jlcpcb/`, with `starter-<version>-gerbers.zip`, `starter-<version>-bom.csv` and `starter-<version>-cpl.csv`. The ZIP includes the project `LICENSE`, `THIRD_PARTY_NOTICES.md` and board `LICENSE-KICAD.md`; copies are also emitted as `starter-<version>-LICENSE`, `starter-<version>-THIRD_PARTY_NOTICES.md` and `starter-<version>-LICENSE-KICAD.md`. Preserve these notices with the exported files. SMD-only output adds `-smd` after the version in every output filename. All fabrication files are regenerated from saved CAD; no prior export folder is needed. Temporary work stays in the output directory and is removed on exit.

Upload the Gerber ZIP for fabrication and the two CSVs separately for assembly. The BOM includes populated through-hole parts unless `--smd-only` is supplied. Mounting holes, DNP and BOM/position exclusions are omitted; both CSVs contain the same references. Blank JLCPCB catalog numbers must be matched using the exact manufacturer part number. Add schematic fields `LCSC` or `JLCPCB` with a `C<number>` identifier for explicit catalog selection. Component-specific rotation corrections are not guessed: check polarity, pin 1 and orientation in the assembly preview.

The script exports the saved filled board without changing it. Run the board’s ERC/DRC/parity checks and review [fabrication requirements](../electronics/boards/starter/0.1.0/FABRICATION.md) before ordering, including the four-layer stackup and USB impedance specification.

Run converter/version-selection checks with `python3 -m unittest discover -s tools -p 'test_jlcpcb.py'`.
