# Firmware build integration

`arm-gcc.cmake` selects the Cortex-M0+ GNU toolchain. `dependencies.cmake` pins firmware-common to a full commit; dependency versions are owned there. Use CMake's `FETCHCONTENT_SOURCE_DIR_FIRMWARE_COMMON` override for local common development. The board and application remain in this device repository.
