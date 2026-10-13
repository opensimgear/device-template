# Native device simulation

Run the device firmware as an independent macOS/Linux process. The simulator supplies a monotonic clock, ADC/contact samples, flash pages and CAN transport. Configuration commands, input acquisition, boot sessions, CAN delivery and HID descriptor generation use the production sources in this repository and its pinned `firmware-common` dependency.

Python 3, Git and a C11 compiler are required. From this repository:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
```

The first launch clones the pinned `firmware-common` revision into `.simulation/deps/`, initializes its submodules and builds `.simulation/osg-device-sim`. Subsequent launches reuse an unchanged build. No adjacent checkout or workspace setup is required. Build and state files stay in this repository's ignored `.simulation/` directory.

Start ordinary Cockpit, then run the gateway from the `device-gateway` repository's `simulation/` folder. Launch several independent device processes:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
python3 simulation/run.py --config simulation/configs/button-panel.json
python3 simulation/run.py --config simulation/configs/shifter.json
```

Run each command in its own terminal. Their example identities differ. Devices may start before the gateway; they wait for it and reconnect automatically. Cockpit discovers them through the gateway's ordinary protocol. Closing Cockpit leaves these processes running. This provides application and firmware integration on local loopback sockets; it does not enumerate a USB/HID peripheral in the OS.

To target another gateway or isolate persistent settings:

```sh
python3 simulation/run.py --config my-device.json \
  --gateway 00112233445566778899aabb \
  --state-dir .simulation/state/my-device
```

Each instance needs a unique 32-character lowercase hexadecimal `identity`. The gateway rejects simultaneously active duplicate identities. State directories are locked before flash access. By default, a device's state is `.simulation/state/<identity>/`. Factory values from JSON initialize fresh state; settings saved through Cockpit take precedence on restart. Editing JSON changes the factory reset target, not already committed settings. Select a fresh state directory to start again from JSON, or use Cockpit's reset and save flow. Never share one device's state directory with a different identity.

## JSON configuration

`identity`, `name` and `inputs` are required. Counts derive from the input arrays. Omitted arrays are empty. Unknown fields, duplicate JSON keys, overlapping mappings, invalid types and out-of-range values are rejected before the process joins CAN. Files must be UTF-8 and at most 64 KiB. The native firmware repeats profile validation and builds its production descriptor before accepting the definition.

```json
{
  "identity": "112233445566778899aabbccddeeff001",
  "name": "Pedals",
  "inputs": {
    "axes": [
      { "channel": 0, "usage": "X", "min": 100, "max": 3900,
        "neutral": 0, "inverted": false, "deadzone": 0 }
    ],
    "buttons": [{ "input": 0, "mode": "momentary" }]
  },
  "initial_samples": { "axes": [100], "closed_contacts": [] }
}
```

| Field | Meaning |
|---|---|
| `name` | Nonempty, at most 63 UTF-8 bytes; no control characters |
| `inputs.buttons` | Up to 128 `{input, mode, exclusive_group}`; mode defaults to `momentary`, or `maintained`; group defaults to 0 and may be 1–255 for mutually exclusive momentary contacts |
| `inputs.axes` | Up to 8 unique `{channel, usage, min, max, neutral, inverted, deadzone}`; channels 0–7; usages `X`, `Y`, `Z`, `Rx`, `Ry`, `Rz`, `Slider`, `Dial`, `Wheel` or numeric HID usages 48–56 |
| Axis defaults | `min: 0`, `max: 4095`, `neutral: 0`, `inverted: false`, `deadzone: 0`; min must be below max; neutral is 0–65535 HID units and deadzone is 0–32767 HID units |
| `inputs.hats` | Up to 4 `{up, right, down, left}` contact mappings |
| `inputs.encoders` | Up to 8 `{a, b}` mappings using direct contacts 0–23; each contributes two HID buttons |
| `inputs.selectors` | Up to 8 `{low, high}` contact mappings; each contributes three HID buttons |
| `expansion_banks` | Enabled TCA9555 bank indexes 0–7; defaults to `[]` |
| `initial_samples.axes` | Raw ADC values 0–4095 in physical channel order; omitted trailing channels start at 0 |
| `initial_samples.closed_contacts` | Contact indexes held closed at startup; defaults to `[]` |
| `initial_samples.expansion_available` | Initially responding bank indexes; defaults to every configured bank |

Contacts 0–23 refer to direct digital lines. Expansion contacts use `24 + bank * 16 + pin` through 151 and require that bank in `expansion_banks`. Every contact is mapped once. Total buttons including encoder/selector outputs must be at most 128. Firmware startup/reconnection suppression still applies to contacts held closed at startup; release and press again to generate a momentary event.

Validate without starting sockets or touching persistent state:

```sh
python3 simulation/run.py --config my-device.json --validate
```

## Interactive inputs and diagnostics

Type commands in the device terminal. Each replies with one JSON line containing the request `id`, `ok` and `simulation` diagnostics. Commands only affect the simulation process; Cockpit uses the normal production configuration protocol through its gateway.

```text
sim 1 inspect
sim 2 axis 0 4095
sim 3 contact 0 1
sim 4 contact 0 0
sim 5 expansion_available 0
sim 6 expansion_available 1
sim 7 reboot
```

`expansion_available` accepts a bit mask for responding banks. `inspect` reports the active configuration, descriptor, HID report bytes, boot counter and CAN diagnostics. Tests can also use `fault <flash-operation-count>` to interrupt a settings write, `can_node <1..127>` to force a node collision, `advance <milliseconds>` to wait while servicing CAN, `direct <0|1>` to simulate direct USB ownership and `direct_config <firmware-command>` to exercise device commands without a gateway. Direct mode changes production CAN ownership; ordinary gateway integration should leave it disabled.

## Build and tests

```sh
python3 simulation/build.py --sanitize
python3 simulation/test.py --sanitize
```

The tests cover example JSON through native firmware validation, invalid/overlapping mappings, complete factory axis options, live sample processing, save, process kill/restart, changed JSON defaults and state locking. Full gateway/Cockpit scenarios are bootstrapped by the initiating repository's own E2E runner.

Integration runners may supply an already initialized dependency checkout and private output directory:

```sh
python3 simulation/build.py --common-source /path/to/firmware-common \
  --output-dir .e2e/build/device-template --sanitize
python3 simulation/run.py --config simulation/configs/pedals.json \
  --output-dir .e2e/build/device-template --no-build \
  --state-dir .e2e/state/pedals
```

`--common-source` is an explicit development override. Ordinary builds use the revision pinned in `firmware/cmake/common-revision.cmake`. `run.py` replaces itself with the native executable, so signals and process IDs target the device directly. The internal executable receives a bounded binary startup definition through an inherited pipe; the public configuration format is JSON.
