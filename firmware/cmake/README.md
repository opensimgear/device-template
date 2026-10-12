# Firmware build integration

`arm-gcc.cmake` selects the Cortex-M0+ GNU toolchain. `dependencies.cmake` pins firmware-common to a full commit; HAL, CMSIS and TinyUSB dependency versions are owned there. Use CMake's `FETCHCONTENT_SOURCE_DIR_FIRMWARE_COMMON` override for local common development. The board and application remain in this device repository.


The application links the `osg_cyphal` target from `../shared-cyphal`, including its vendored libcanard, generated Nunavut bindings and fixed-memory transport implementation. This checked-in snapshot is synchronized with canonical `protocol/cyphal` and the matching gateway snapshot; its manifest detects drift. Builds do not require Python or a DSDL compiler. Regenerate the canonical bindings and synchronize both firmware copies together when changing the Cyphal contract. Retain the generated-runtime and regulated-type notices listed in [third-party notices](../THIRD_PARTY.md).
