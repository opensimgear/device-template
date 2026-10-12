# Rev B input-header footprint review

The analogue, digital and expansion connectors use Samtec TSW-103-07-L-S, TSW-107-07-L-S and TSW-104-07-L-S respectively. They are unkeyed 2.54 mm pin headers, intended for internal enclosure wiring. Signal numbering and the square pin-1 land determine orientation; they provide no protection against reversed plugs.

The existing reviewed TSW series drawing applies to the new seven-position footprint. All pins use 1.02 mm finished holes, 1.70 mm copper lands and 2.54 mm pitch for 0.635 mm square posts. The seven-position row spans 15.24 mm between the first and last pin centres; its nominal moulded body is 2.54 × 17.78 mm. The footprint has a 0.28 mm courtyard allowance on each body edge. No header lands were reduced to increase density.

Manufacturer references: [recommended footprint](https://suddendocs.samtec.com/prints/tsw-xxx-xx-x-x-xx-xxx-footprint.pdf), [TSW product drawing](https://suddendocs.samtec.com/prints/tsw-xxx-xx-xxx-x-xx-xxx-mkt.pdf). The previously archived primary drawings are retained under `.research/pcb-review-datasheets` for analysis.

This is a drawing review. Actual purchased parts must pass dry-fit and solderability checks before fabrication release.
