# Independent 4P4C connector review

The selected Evercom 5301-4P4C (LCSC C3097715) matches manufacturer drawing **5301-440XXX Rev A, 2025-09-23**. Pixel review confirms the component-side numbering, four 0.90 mm contact holes and two 3.20 mm locating holes. The two post centers are 7.50 mm apart. The exact evidence and dimensions are in `evercom-4p4c-geometry.json`; the primary drawing is archived under `.research/gateway-datasheets/evercom-5301.pdf`.

**One source-library correction:** the right post had a 3.25 mm drill; both must use 3.20 mm. The 1.80 mm contact copper lands are the project design choice, not a manufacturer-specified pad diameter.

The manufacturer PDF is served through Documents on the [stable JLC product page](https://jlcpcb.com/partdetail/EVERCOM-53014P4C/C3097715), using an expiring download URL. The unsigned download returns 403 and the advertised LCSC PDF address returns HTML. Do not publish either as a verified direct source. Archive SHA256: `870bbd7e703e2c971a4a50ef716cff00093e3872e34a4494434166ad0d645695`.

The obsolete Amphenol alternative was also verified and archived; its 7.62 mm post span and 14.15 mm nose offset must not be mixed with Evercom. This review does not certify cabling, hot plugging or current capacity of the complete chain.
