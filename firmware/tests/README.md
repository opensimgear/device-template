# Firmware validation

Run the native test commands in [firmware README](../README.md). A delivery-state test covers retained and canceled USB transfers, generation acknowledgement and resume readiness. All four supplied profiles have behavior and descriptor tests, including exact field omission and report length. Shared tests cover profile bounds, collisions, axis-only/button-only/hat-only descriptors, conflict groups, cross-bank faults, stale input and expansion freshness, time wraparound and acknowledged encoder pulses.

The board targets support cross-building `example`, `shifter`, `expanded` and `master_caution` for `starter_g0b1cb_v0_1_0`. [v0.1.0 validation](../../electronics/boards/starter/0.1.0/VALIDATION.md) records CAD and physical gates. Native and compile checks do not replace physical USB, ADC, encoder timing or I²C fault tests.

The CAN client suite connects the device to real Cyphal peers and exercises assignment/timeout, lossless maximum profiles, exact gateway USB completion ACKs, gateway source binding, transport node-ID collision recovery, stale boot/lease/sequence/duplicate/wrong-report rejection, direct USB priority, per-encoder phase sequence guards and interrupted boot-counter journal writes. Profile behavior tests also compare each supplied profile with the generated Cyphal report length and exact neutral bytes. See [CAN validation](../CAN_BRINGUP.md).

The shared transport suite runs the canonical tests against the device's
snapshot, including generated DSDL, FD/Classic transfers, node services,
collision recovery, queue deadlines and bounded allocation.

The runtime configuration suite runs the production core with simulated flash. It covers every control kind, complete-profile validation, writer conflict/expiry/wraparound, draft discard/reset, pending versus active descriptors, calibration/inversion/deadzone, maximum snapshots, canonical storage and corrupt settings. Initialization commits factory defaults as saved generation 1; tests interrupt each of its 80 erase/program operations, including a second interruption while recovering, then reboot and retry. Both first-user `apply_save` and `save` are interrupted at each of their 80 operations and preserve the baseline on reboot. Invalid committed records are preserved and rejected. Each profile HID suite also switches to a runtime axis-only descriptor through the actual USB callbacks and checks Unicode UTF-16 interface strings. Hardware flash/USB timing remains outside these tests.

The advertised-action suite uses the production registry for the current board's `auto/on/off` status LED and the simulation's RGB LED 1/boolean LED 2. It covers descriptors, readback, argument and stale-catalogue rejection, reboot defaults, suspend behavior, CAN chunk assembly, immutable retry replies and current lease invalidation. Actions leave configuration generation, descriptor replacement and pending activation untouched. The CAN client also proves its assigned lease without an input report and under direct USB ownership. Native simulation tests verify action persistence across ownership changes, volatile reboot reset and unchanged flash-write/descriptor/reconnect counters.

The action/CAN suite also routes production `events state/read` commands, checks
immutable cached snapshots after later input, stale-stream rejection and lease
cache invalidation, and verifies no settings activation or claim occurs. The
Master Caution process test captures a debounced press/release before any read,
pages a wrapped 64-record log, reports gaps/future cursors, retains events across
direct USB handoff, and resets stream/held state on configuration activation and
reboot. It uses the board LED action and checks unchanged settings-write,
descriptor and reconnect counters for reads/actions. An offline Apply regression
waits for actual activation rather than a fixed sleep. Shared event/encoder tests
and their coverage gate are maintained in firmware-common.

Clang/LLVM line coverage for each of `device_actions.c`, `config_can.c` and `can_client.c` must remain greater than 70%. CI enforces this with:

```sh
python3 firmware/tests/coverage.py
```

The runner rebuilds the native suites with coverage instrumentation, executes all tests and stores raw profiles, `coverage.profdata` and `coverage.json` under `../.research/device-template-coverage/`. Use `--build-dir` for a different scratch directory. Shared-code development accepts `--common-source /path/to/firmware-common` and `--tinyusb-source /path/to/tinyusb`. Clang and LLVM must use compatible versions; `--clang`, `--llvm-cov` and `--llvm-profdata` select explicit binaries. macOS also discovers the tools through `xcrun`.
