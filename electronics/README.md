# Electronics

The [Rev B starter board](boards/starter/rev-b/README.md) exposes eight analogue inputs, 24 digital lines and I²C for optional [16-input TCA9555 boards](expansion/tca9555/README.md). It includes editable KiCad 10 sources, local libraries, an exact-part BOM and separate production/home-milling layouts. The validation record distinguishes static checks from assembled-board qualification.

Each electrical revision lives under `boards/<board>/<revision>`; mechanical editions can share it. [Rev A](boards/starter/rev-a/README.md) preserves the earlier example-control circuit and its reviewed layouts. Rev A and Rev B require their matching firmware pin maps.
