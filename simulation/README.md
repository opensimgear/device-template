# Native device simulation

Run the device firmware as an independent macOS/Linux process. The simulator supplies a monotonic clock, ADC/contact samples, flash pages, RGB LED 1/boolean LED 2 or the board status LED action, a bounded panel event log and CAN transport. Configuration commands, advertised actions, input acquisition, boot sessions, CAN delivery and HID descriptor generation use the production sources in this repository and its pinned `firmware-common` dependency.

Python 3, Git and a C11 compiler are required. From this repository:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
```

The first launch clones the pinned `firmware-common` revision into `.simulation/deps/`, initializes its submodules and builds `.simulation/osg-device-sim`. Subsequent launches reuse an unchanged build. When the dependency pin changes, the builder updates a clean cached checkout automatically, fetching the revision if needed. Local changes in a stale cache are preserved and reported; use `--common-source` for an intentionally edited dependency. No adjacent checkout or workspace setup is required. Build and state files stay in this repository's ignored `.simulation/` directory.

Start ordinary Cockpit, then run the gateway from the `device-gateway` repository's `simulation/` folder. Launch several independent device processes:

```sh
python3 simulation/run.py --config simulation/configs/pedals.json
python3 simulation/run.py --config simulation/configs/button-panel.json
python3 simulation/run.py --config simulation/configs/shifter.json
python3 simulation/run.py --config simulation/configs/master-caution.json
python3 simulation/run.py --config simulation/configs/radio-panel.json
```

Run each command in its own terminal. Their example identities differ. Devices may start before the gateway; they wait for it and reconnect automatically. Cockpit discovers them through the gateway's ordinary protocol. Closing Cockpit leaves these processes running. This provides application and firmware integration on local loopback sockets; it does not enumerate a USB/HID peripheral in the OS.

To target another gateway or isolate persistent settings:

```sh
python3 simulation/run.py --config my-device.json \
  --gateway 00112233445566778899aabb \
  --state-dir .simulation/state/my-device
```

Each instance needs a unique 32-character lowercase hexadecimal `identity`. The gateway rejects simultaneously active duplicate identities. State directories are locked before flash access. By default, a device's state is `.simulation/state/<identity>/`. Factory values from JSON initialize fresh state; settings saved through Cockpit take precedence on restart. Editing JSON changes the factory reset target, not already committed settings. Select a fresh state directory to start again from JSON, or use Cockpit's reset and save flow. Never share one device's state directory with a different identity.

For isolated tests, set the same `OSG_SIMULATION_NAMESPACE` on the gateway, device processes and the Cockpit service. It selects `/tmp/opensimgear-<uid>/gateways-<namespace>/` instead of the ordinary `gateways/` directory, so a separately running Cockpit cannot attach to the test gateway. Names contain 1–31 ASCII letters, digits, `.`, `_` or `-`; invalid names are rejected before state access. Omit the variable for ordinary automatic discovery. For example:

```sh
OSG_SIMULATION_NAMESPACE=actions-ui python3 simulation/run.py --config simulation/configs/pedals.json
```

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
| `outputs` | `"simulated-leds"` (default RGB/boolean), `"board-status"`, or an object with `leds` and `displays` as described below |
| `initial_samples.axes` | Raw ADC values 0–4095 in physical channel order; omitted trailing channels start at 0 |
| `initial_samples.closed_contacts` | Contact indexes held closed at startup; defaults to `[]` |
| `initial_samples.expansion_available` | Initially responding bank indexes; defaults to every configured bank |

Contacts 0–23 refer to direct digital lines. Expansion contacts use `24 + bank * 16 + pin` through 151 and require that bank in `expansion_banks`. Every contact is mapped once. Total buttons including encoder/selector outputs must be at most 128. Firmware startup/reconnection suppression still applies to contacts held closed at startup; release and press again to generate a momentary event.

Validate without starting sockets or touching persistent state:

```sh
python3 simulation/run.py --config my-device.json --validate
```

## Fitted display ports

The radio example supplies `outputs: {"leds": "board-status", "displays": [...]}`.
There are at most four display definitions. Each declares unique ASCII `id` and
`endpoint` (1–31 bytes), a UTF-8 `label` (1–63 bytes, no controls), `columns`
(1–63), optional `rows` (1–4, default 1), `encoding` (`ascii` default or `utf8`)
and `max_bytes` (surface size through 63). Columns × rows is at most 63 cells.
ASCII defaults to one byte per cell; UTF-8 defaults to 63 bytes, with at most one
Unicode scalar per cell. The flattened surface is row-major; UTF-8 cells are not
a promise about grapheme width or fitted fonts. Control characters and invalid
UTF-8 are rejected by the production action validator.

The port definitions generate ordinary readable one-string setters carrying
`display` geometry/encoding metadata and `parameters[0].maxBytes`. Native write/read
callbacks own the actual buffers; `simulation.displays` reports text and successful
write counts from those buffers. Width/encoding errors never invoke the driver.
Values are volatile and reset empty on reboot. Output capabilities are product
definitions, separate from saved input settings. A physical port supplies the
same firmware interface; the starter board reports zero fitted displays and does
not advertise an LCD. The native radio provides 7-cell ASCII frequency, 2-cell
ASCII preset and 16-cell UTF-8 text ports. For example:

```text
sim 1 direct_action invoke 1 display.uhf.frequency.set_text 3235312e303030
sim 2 direct_action invoke 1 display.uhf.preset.set_text 3230
sim 3 direct_action invoke 1 display.text.set_text c3a9e6b3a2
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
sim 8 direct_action describe 0
sim 9 direct_action invoke 1 led.rgb.set_color 12ABEF
sim 10 direct_action invoke 1 led.boolean.set_enabled 1
sim 11 direct_action state
sim 12 direct_events state
sim 13 direct_events read 1 0
```

`expansion_available` accepts a bit mask for responding banks. `inspect` reports the active configuration, descriptor, HID report bytes, boot counter and CAN diagnostics. Tests can also use `fault <flash-operation-count>` to interrupt a settings write, `can_node <1..127>` to force a node collision, `advance <milliseconds>` to wait while servicing CAN, `direct <0|1>` to simulate direct USB ownership and `direct_config <firmware-command>` to exercise device commands without a gateway. Direct mode changes production CAN ownership; ordinary gateway integration should leave it disabled.

For the default output profile, Cockpit discovers the simulator's two actions and renders their typed manual controls. RGB is six hexadecimal digits; the boolean parameter is `0` or `1`. `simulation.actions` contains readback, and `simulation.actionResult` contains the latest direct action result. These setters remain usable with a disabled HID controller or direct USB ownership. They update volatile output state without a configuration writer claim, flash writes, HID changes or reconnects. Ownership changes retain the current values; device reboot resets RGB to `000000` and the boolean LED to off. Invalid IDs, arguments and catalogue revisions return an error and retain previous values. The simulation uses the shared production action registry, while the real board advertises its actual monochrome status LED.

The `master-caution.json` example maps contact D0 to momentary logical control 0
and selects the same `led.status.set_mode` catalogue as the board. Use
`direct_action invoke 1 led.status.set_mode on` (or `off`/`auto`) with this
profile. Reboot restores `auto`; these are readable software mode values.

`simulation.events` exposes stream/head/oldest/overflow/held metadata;
`simulation.eventResult` retains the latest `direct_events` result. Acquisition
runs without HID ownership and captures debounced button transitions and encoder
detent press/release pairs in a 64-record ring, with nonconsuming 16-record pages.
Take a state baseline and use its `stream`/`head` as the next read scope/cursor;
rebaseline on a gap, mapping activation or reboot. The JSON reply is the same
production contract routed as `targetEvents` through the gateway. Fresh reads can change observation metadata while retained event records stay
immutable; transport retries retain the whole original response. Reads leave
configuration, flash and HID unchanged. See the [event contract](https://github.com/opensimgear/protocol/blob/main/serial/panel-input-events.md).

Radio observations additionally expose plain-contact count, maintained/selector
indexes, declared axis usages/neutrals, calibrated 16-bit values, availability and
module-local `sampledAtMs`. The collector runs its own acquisition and the same
production inversion/deadzone transform as HID. `axes_available <mask>` simulates
ADC freshness; after 100 ms without a sample an axis is unavailable and neutral.
`display_fail <index> <0|1>` and `display_read_fail <index> <0|1>` inject port
callback failures. Readback failure after a successful write is an uncertain
result; inspect/refresh instead of replaying an output. These diagnostics are
simulator-only, while Cockpit uses ordinary routed commands.

`turn <encoder ordinal> <signed detents>` traverses real sampled quadrature
edges at 2 ms intervals, returning both contacts to their open idle position.
It accepts 1–128 detents in either direction; it does not inject logical events.
`display_delay <port index> <0..250 ms>` models a bounded synchronous driver
delay inside the native write callback. Acquisition is never called recursively
from that callback. Inspect reports each native port's successful write count
and selected delay. Reboot clears these transient delay/failure settings.

## Build and tests

```sh
python3 simulation/build.py --sanitize
python3 simulation/test.py --sanitize
```

The tests cover example JSON through native firmware validation, invalid/overlapping mappings, complete factory axis options, live sample processing, save, process kill/restart, changed JSON defaults and state locking. Action tests cover descriptors, RGB/boolean readback, validation, volatile restart defaults, ownership changes and unchanged settings-write/descriptor/reconnect counters. Event tests cover Master Caution board-compatible output metadata, short presses between reads, immutable retained records, ring overflow/gaps, direct USB handoff, offline configuration activation and boot/stream reset. Radio tests additionally cover native display callbacks/geometry, ASCII and UTF-8 cell/byte bounds, callback failures, maintained/encoder input, independent calibrated axes, stale ADC recovery and reboot defaults. Full gateway/Cockpit scenarios are bootstrapped by the initiating repository's own E2E runner.

Integration runners may supply an already initialized dependency checkout and private output directory:

```sh
python3 simulation/build.py --common-source /path/to/firmware-common \
  --output-dir .e2e/build/device-template --sanitize
python3 simulation/run.py --config simulation/configs/pedals.json \
  --output-dir .e2e/build/device-template --no-build \
  --state-dir .e2e/state/pedals
```

`--common-source` is an explicit development override. Ordinary builds use the revision pinned in `firmware/cmake/common-revision.cmake`. `run.py` replaces itself with the native executable, so signals and process IDs target the device directly. The internal executable receives a bounded binary startup definition through an inherited pipe; the public configuration format is JSON.

## Output-only devices and endpoint leases

Run `configs/output-only.json` for the production RGB LED 1/boolean LED 2 callbacks
or `configs/output-only-board-led.json` for the board's actual status-mode action.
An empty `inputs: {}` is valid: direct USB has no HID descriptor and the gateway
enrolls the device with guarded routes, no report ID and no controller collection.
The same device can later activate configured inputs through normal Apply.

All simulated output registries advertise the production endpoint ownership
capability. Claims store typed fallback without writing it; USB/CAN scopes and
secret UUIDs prevent competing writers. Lease TTL is 250–5000 ms. For example:

```text
sim 1 direct_action claim 1 led.rgb.set_color 00112233-4455-6677-8899-aabbccddeeff 5000 000000
sim 2 direct_action invoke_owned 00112233-4455-6677-8899-aabbccddeeff 1 1 led.rgb.set_color 12abef
sim 3 direct_action renew 00112233-4455-6677-8899-aabbccddeeff 1 5000
sim 4 direct_action ownership 0
sim 5 direct_action release 00112233-4455-6677-8899-aabbccddeeff 1
```

Use a fresh secret per host session; the example token is illustrative. Public
ownership never returns tokens. `advance` runs the independent clock/expiry loop,
`usb_close` invalidates direct USB claims, and reboot restores product defaults.
CAN route changes invalidate only CAN claims. Legacy actions and `led on/off/auto`
are blocked on claimed endpoints. Different endpoints remain independent.
Claims, writes and expiry leave flash, settings, HID descriptors and reconnect
counters unchanged. See the [full contract](https://github.com/opensimgear/protocol/blob/main/serial/output-ownership.md).
The eight native process suites include both output-only presets, competing
owners, stale leases, idle expiry, disconnect, reboot and unchanged persistence.
