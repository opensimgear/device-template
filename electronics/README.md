# Electronics

The [v0.1.0 starter board](boards/starter/0.1.0/README.md) exposes eight analogue inputs, 24 digital lines and I²C for optional [16-input TCA9555 boards](expansion/tca9555/README.md). It includes editable KiCad 10 sources, local libraries, an exact-part BOM and separate production/home-milling layouts. The validation record distinguishes static checks from assembled-board qualification.

One current design lives under `boards/<board>/0.1.0/`; earlier circuits remain in Git history. [PCB versioning](VERSIONING.md) defines MAJOR.MINOR.PATCH and keeps the current version fixed until launch. Mechanical editions and firmware must document their compatibility with this PCB.
