# OpenSimGear firmware template

GitHub template for a **complete device repository**, with firmware/, hardware/, electronics/, and a root README. Use it to start a new device while keeping each device's firmware and physical design together.

## Status

The repository layout is initialized. The buildable STM32G0B1 hello-world firmware target is the next implementation step; this scaffold does not yet enumerate USB, communicate over CAN, or produce a flashable image.

The planned example uses a documented real STM32G0B1 reference board, shared firmware-common code, HID plus CDC serial, and an explicit test input. It must not synthesize automatic simulator commands. Reference-board wiring and recovery instructions must exist before advertising the example as buildable.

## Create a device

1. Select **Use this template** in GitHub and choose the new device repository name.
2. Replace this README with the device overview, control behavior, and status.
3. Define the device model and capability profile; select a real board target. Allocate valid public USB identifiers before distribution and keep each unit's identity unique.
4. Pin a tested firmware-common commit through firmware/cmake/dependencies.cmake when build integration is implemented.
5. Add mechanical designs under hardware/variants/<edition>/<revision> and PCB designs under electronics/boards/<board>/<revision>.
6. Document a compatibility table mapping editions to mechanical revisions, PCB revisions, and tested firmware versions. Remove unused edition placeholders from the new device.

## Layout

- [firmware/](firmware/README.md): device-specific C sources, targets, profiles, and tests.
- [hardware/](hardware/README.md): Lite/Pro edition folders and future mechanical sources.
- [electronics/](electronics/README.md): shared PCB and wiring sources.

Keep common electronics and firmware once. A new enclosure material alone does not require a new USB identity or duplicate firmware. Updates to this template do not automatically update repositories created from it; reusable implementation stays in firmware-common.

## Related repositories

- [Protocol](https://github.com/opensimgear/protocol): shared definitions and compatibility rules.
- [Firmware common](https://github.com/opensimgear/firmware-common): reusable C implementation.
- [Firmware template](https://github.com/opensimgear/firmware-template): starting layout for new devices.
- [Cockpit](https://github.com/opensimgear/cockpit): desktop configuration application.
- [Website](https://github.com/opensimgear/opensimgear.github.io): system architecture and user documentation.

## Licensing

License selection is pending. No license grant is implied by this scaffold. Firmware, mechanical design, and electronics will each need documented license coverage before releases and any public USB identifier application.
