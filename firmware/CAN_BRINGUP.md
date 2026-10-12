# Generic device CAN integration

The generic Rev B firmware is a **Cyphal/CAN device** running the shared libcanard stack at 500 kbit/s arbitration / 2 Mbit/s data. Generated OpenSimGear DSDL transfers replace the old private CAN frame codec. Both sockets use the existing 4P4C wiring: 1=BUS_12V, 2=CAN_H, 3=CAN_L, 4=GND. Endpoint termination and cable/current qualification remain hardware requirements.

## Discovery and reports

The device operates independently of a gateway: it selects a Cyphal transport node ID, publishes the standard `uavcan.node.Heartbeat.1.0` message, and responds to `uavcan.node.GetInfo.1.0` with the name `org.opensimgear.device` and its stable unique ID. Transport node IDs are separate from the gateway's logical controller slots 1–10. Automatic node-ID selection is provided by the pinned Cyphal 1.1 libcanard implementation; it is not a private gateway allocation service.

The module identity is 16 bytes: ASCII `OSGD` followed by the MCU's 12 raw UID bytes. Its USB serial remains the original 24 hexadecimal UID characters. The module commits a fresh nonzero boot counter before starting its announcement loop. The typed capability announcement repeats at a nominal 500 ms (450–550 ms with identity/session seeded jitter) and includes the enabled HID profile, direct USB ownership and boot session. A matching gateway returns a logical controller slot, lease and gateway epoch in a typed assignment transfer. The device records the assigning gateway's actual Cyphal source node ID and rejects assignment/ACK transfers from another source while that route is live. An unchanged live assignment is idempotent; a new lease rearms acquisition. Assignment absence for 2 s invalidates the route and returns to discovery.

CAN input uses the same report layout as direct USB: up to 128 logical button bits, four packed hats and eight unsigned 16-bit axes, at most 34 bytes. All supplied profiles are lossless; no controls are silently removed. Invalid profiles fail before attachment. Unused padding is zero, and hats use 8 for neutral. Typed input normally refreshes every 10 ms while a current CAN assignment owns the route. A new route first sends neutral and requires fresh input acquisition.

## Ownership and encoder delivery

Mounted, awake direct USB has priority. While it owns input, announcements advertise `direct_usb=true` and the gateway neutralizes the routed slot. Detach/suspend allows a fresh CAN fallback. Each ownership transition discards queued encoder events and stale report acknowledgements, clears pending CAN traffic through a bounded controller reset, then rearms held controls. Normal ownership changes restart immediately and send a priority ownership announcement. A real bus fault or failed restart waits 1 s before retrying. The direct USB HID completion behavior and retained-packet caveat in [README](README.md) still apply.

Successful CAN transmission does not acknowledge an encoder pulse. The module waits for the typed input acknowledgement emitted only after the gateway's actual USB HID completion. ACK must come from the assigned gateway source and match the current boot, logical controller slot and lease; its sequence and exact report must be present in the bounded 16-report transmit history. Duplicate, older, unknown and mismatched acknowledgements are rejected. Each encoder phase records a minimum input sequence, so a delayed ACK for an earlier same-direction press cannot acknowledge a later press. Press and release hold times start only after their corresponding completion ACK. If USB delivery stalls, encoder queues remain bounded and phases wait; there is no synthetic delivery credit.

The application stack emits extended FD+BRS frames. Hardware accepts all extended data frames so standard Cyphal messages, services and automatic node-ID selection can reach libcanard; classical CAN and FD frames without BRS are also received. RX processes at most four frames per loop. libcanard performs transfer-ID handling, duplicate suppression, multi-frame CRC validation and reassembly before application processing. Its bounded transmit queue retains transfers while the board queue is full. The 32-frame board transmit queue retains each transfer's deadline, discards expired queued frames, and cancels an expired active mailbox before reusing its queue head. Live frames permit at most three attempts, 20 ms retry spacing and 20 ms mailbox timeout; cancellation that cannot complete within 20 ms triggers recovery. Ownership/recovery resets purge both transport and board queues. Bus-off invalidates assignment and re-enters discovery after recovery. A transport node-ID collision invalidates the logical route and transmit history, purges old-source frames from the controller, and forces discovery under the newly selected Cyphal ID. Traffic from an old gateway source/lease/session cannot advance encoders. Lease ordering is checked within the current gateway epoch, and timeout/reset clears that scope. The protocol does not authenticate bus participants.

## Boot freshness and memory

The last two 2 KiB flash pages, at `0x0801F000` and `0x0801F800`, contain a CRC-protected two-page boot journal. The commit marker is programmed last. One interrupted write preserves the prior valid record; the next successful boot advances from it before announcing. A virgin all-FF journal starts at session 1. Corruption with no valid record, an exhausted counter, or failed erase/program causes a fail-closed fault with CAN standby asserted. Boot writes consume flash endurance; no write occurs per report or discovery.

Firmware code is restricted to 124 KiB flash and uses guaranteed 128 KiB SRAM, reserving 16 KiB stack and 512 bytes heap. Linker assertions prevent overlap with storage. Preserve the boot pages when programming an application update. If a full flash erase resets boot history, power-cycle the entire chain before resuming so no cached session remains. Runtime calibration and physical input configuration are still build-time declarations; this journal stores only boot freshness.

## Validation on 2026-10-12

- Native Release and AddressSanitizer/UndefinedBehaviorSanitizer: **9/9 tests pass**, including all three real profile descriptors, report compatibility, USB delivery recovery, the Cyphal client/encoder/journal suite and shared transport conformance.
- Cyphal client tests use actual transport peers and cover gateway source binding, boot/lease/sequence/report validation, direct USB ownership, bounded queue rejection, assignment timeout, sequence exhaustion, late same-direction phase ACKs and interrupted boot-counter writes.
- All three profiles cross-build with Arm GNU 15.3.1 for STM32G0B1CBT6. Compilation uses `0xFFFF/0xFFFF` placeholders only; no firmware was flashed.
- GCC stack usage is emitted as `.su` files. The boot journal commit frame is 2,168 bytes plus its 2,016-byte CRC scratch frame, below the 16 KiB reserved stack with the main/port frames. This is static evidence, not measured peak stack.

| Profile | Flash | SRAM including reserved heap/stack |
| --- | ---: | ---: |
| example | 55,472 B | 60,176 B |
| shifter | 55,456 B | 60,176 B |
| expanded | 55,632 B | 60,176 B |

Native reproduction from the repository root:

```sh
cmake -S firmware/tests -B ../.research/device-template/tests -DCMAKE_BUILD_TYPE=Release
cmake --build ../.research/device-template/tests --parallel
ctest --test-dir ../.research/device-template/tests --output-on-failure
```

No assembled module/gateway CAN chain, USB suspend/fallback timing, ADC/I²C/encoder timing, signal margins or power behavior was tested. Bench qualification must verify discovery, all enabled controls including hats, gateway registry/apply/reconnect, cable removal, bus-off recovery, direct USB takeover/fallback, lost/delayed acknowledgements and no replay after reset. Runtime physical settings, output routing and firmware updates remain future work.
