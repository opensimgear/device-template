#!/usr/bin/env python3
"""Exercise JSON startup, production configuration and flash restart behavior."""
import argparse
import copy
import json
import os
from pathlib import Path
import selectors
import subprocess
import sys
import tempfile
import time
import unittest
sys.dont_write_bytecode = True
from build import build, REPO
from configuration import ConfigurationError, encode_configuration, read_configuration

BINARY_DIR = REPO / '.simulation'


class ConfigurationTests(unittest.TestCase):
    def setUp(self):
        self.pedals = json.loads((REPO / 'simulation/configs/pedals.json').read_text())
        self.environment = {**os.environ, 'OSG_SIMULATION_NAMESPACE': f'device-tests-{os.getpid()}'}

    def test_examples_pass_native_validation(self):
        for name in ('pedals', 'button-panel', 'shifter', 'master-caution', 'radio-panel'):
            result = subprocess.run([sys.executable, str(REPO / 'simulation/run.py'), '--config',
                                     str(REPO / f'simulation/configs/{name}.json'), '--output-dir', str(BINARY_DIR),
                                     '--no-build', '--validate'], capture_output=True, text=True,
                                    env=self.environment)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('production profile are valid', result.stdout)

    def test_invalid_and_overlapping_fields_rejected(self):
        mutations = [
            lambda p: p.update(name='bad\nname'),
            lambda p: p.update(identity='0' * 32),
            lambda p: p.update(extra=True),
            lambda p: p.update(outputs='unsupported'),
            lambda p: p['inputs']['axes'][0].update(channel=True),
            lambda p: p['inputs']['axes'][0].update(inverted=1),
            lambda p: p['inputs']['axes'][0].update(deadzone=32768),
            lambda p: p['inputs']['axes'][0].update(usage='unsupported'),
            lambda p: p['inputs']['axes'][0].update(channel=1),
            lambda p: p['inputs'].update(buttons=[{'input': 24}]),
            lambda p: p['inputs'].update(buttons=[{'input': 0}, {'input': 0}]),
            lambda p: p['inputs'].update(buttons=[{'input': 0, 'mode': 'maintained', 'exclusive_group': 1}]),
        ]
        for mutation in mutations:
            value = copy.deepcopy(self.pedals); mutation(value)
            with self.assertRaises(ConfigurationError):
                encode_configuration(value)
        radio = json.loads((REPO / 'simulation/configs/radio-panel.json').read_text())
        for mutation in (
            lambda p: p['outputs'].update(extra=True),
            lambda p: p['outputs'].update(displays=p['outputs']['displays'] * 2),
            lambda p: p['outputs']['displays'][0].update(columns=0),
            lambda p: p['outputs']['displays'][0].update(columns=32, rows=2),
            lambda p: p['outputs']['displays'][0].update(encoding='latin1'),
            lambda p: p['outputs']['displays'][0].update(max_bytes=6),
            lambda p: p['outputs']['displays'][0].update(max_bytes=64),
            lambda p: p['outputs']['displays'][0].update(label='bad\x85label'),
            lambda p: p['outputs']['displays'][0].update(id='led.status.set_mode'),
            lambda p: p['outputs']['displays'][0].update(id='led.rgb.set_color'),
            lambda p: p['outputs']['displays'][0].update(id='led.boolean.set_enabled'),
            lambda p: p['outputs']['displays'][1].update(endpoint=p['outputs']['displays'][0]['endpoint']),
        ):
            value = copy.deepcopy(radio); mutation(value)
            with self.assertRaises(ConfigurationError):
                encode_configuration(value)
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            path = Path(temporary) / 'duplicate.json'
            path.write_text('{"identity":"a", "identity":"b"}')
            with self.assertRaisesRegex(ConfigurationError, 'duplicate JSON field'):
                read_configuration(path)
            path.write_text(' ' * 65537)
            with self.assertRaisesRegex(ConfigurationError, 'exceeds'):
                read_configuration(path)

    def test_simulation_namespace_is_bounded_and_validated_before_state_access(self):
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            state = Path(temporary) / 'untouched-state'
            command = [sys.executable, str(REPO / 'simulation/run.py'), '--config',
                       str(REPO / 'simulation/configs/pedals.json'), '--output-dir', str(BINARY_DIR),
                       '--no-build', '--state-dir', str(state), '--validate']
            for namespace in ('', 'bad/../path', 'a' * 32, 'white space', 'é'):
                result = subprocess.run(command, capture_output=True, text=True,
                                        env={**self.environment, 'OSG_SIMULATION_NAMESPACE': namespace})
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn('OSG_SIMULATION_NAMESPACE must contain 1..31 ASCII', result.stderr)
                self.assertFalse(state.exists())
            for namespace in ('Z09._-', 'a' * 31):
                result = subprocess.run(command, capture_output=True, text=True,
                                        env={**self.environment, 'OSG_SIMULATION_NAMESPACE': namespace})
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertFalse(state.exists())

    def test_factory_save_restart_reset_and_samples(self):
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            directory = Path(temporary)
            config = directory / 'device.json'
            self.pedals['identity'] = '6ea15ffe17734f2b81a12a12e113aea1'
            self.pedals['inputs']['axes'][1]['deadzone'] = 1000
            config.write_text(json.dumps(self.pedals))
            command = [sys.executable, str(REPO / 'simulation/run.py'), '--config', str(config),
                       '--output-dir', str(BINARY_DIR), '--no-build', '--state-dir', str(directory / 'state'),
                       '--gateway', '8ea15ffe17734f2b81a12a12']
            process = None
            request = 0

            def start():
                return subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True, bufsize=1, env=self.environment)

            def admin(operation, timeout=4):
                nonlocal request
                request += 1
                process.stdin.write(f'sim {request} {operation}\n'); process.stdin.flush()
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    self.assertTrue(selector.select(timeout), f'timed out: {operation}')
                line = process.stdout.readline()
                if not line:
                    self.fail(f'process exited {process.poll()}: {process.stderr.read()}')
                result = json.loads(line)
                self.assertTrue(result['ok'], result)
                return result['simulation']

            def wait_for_applied(result, name, generation):
                deadline = time.monotonic() + 4
                while (result['module']['pendingApply'] or
                       result['module']['generation'] != generation or
                       result['module']['savedGeneration'] != generation or
                       result['module']['config']['name'] != name):
                    remaining = deadline - time.monotonic()
                    self.assertGreater(remaining, 0, f'apply did not settle: {result["module"]}')
                    result = admin('inspect', timeout=remaining)
                return result

            def stop(kill=False):
                if process.poll() is None:
                    process.kill() if kill else process.terminate()
                process.wait(4)
                errors = process.stderr.read()
                self.assertNotIn('AddressSanitizer', errors)
                self.assertNotIn('runtime error:', errors)
                for stream in (process.stdin, process.stdout, process.stderr):
                    stream.close()

            try:
                process = start()
                first = admin('inspect')
                self.assertEqual(first['module']['config']['axes'][1][5:], [1, 1000])
                admin('direct 1')
                report = admin('axis 0 3900')['moduleReportHex']
                self.assertEqual(report[:4], 'ffff')
                admin('direct_config claim 1')
                admin('direct_config name 1 536176656420706564616c73')
                saved = wait_for_applied(admin('direct_config apply_save 1'),
                                         'Saved pedals', first['module']['generation'] + 1)
                self.assertEqual(saved['module']['config']['name'], 'Saved pedals')
                stop(kill=True)
                # Altered JSON remains a reset target; committed settings win.
                self.pedals['name'] = 'Changed JSON defaults'
                config.write_text(json.dumps(self.pedals))
                process = start()
                restored = admin('inspect')
                self.assertGreater(restored['moduleBoot'], first['moduleBoot'])
                self.assertIn('Saved pedals', json.dumps(restored['module']))
                duplicate = subprocess.run(command, input='', capture_output=True, text=True, timeout=4,
                                           env=self.environment)
                self.assertNotEqual(duplicate.returncode, 0)
                self.assertIn('already in use', duplicate.stderr)
                admin('direct 1')
                admin('direct_config claim 2')
                admin('direct_config reset 2')
                reset = wait_for_applied(admin('direct_config apply_save 2'),
                                         'Changed JSON defaults', restored['module']['generation'] + 1)
                self.assertEqual(reset['module']['config']['name'], 'Changed JSON defaults')
            finally:
                if process is not None and not process.stdout.closed:
                    stop()

    def test_master_caution_events_capture_between_reads_and_rebaseline(self):
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            command = [sys.executable, str(REPO / 'simulation/run.py'), '--config',
                       str(REPO / 'simulation/configs/master-caution.json'), '--output-dir', str(BINARY_DIR),
                       '--no-build', '--state-dir', str(Path(temporary) / 'state'),
                       '--gateway', '8ea15ffe17734f2b81a12a12']
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, bufsize=1, env=self.environment)
            request = 0

            def admin(operation):
                nonlocal request
                request += 1
                process.stdin.write(f'sim {request} {operation}\n'); process.stdin.flush()
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    self.assertTrue(selector.select(4), f'timed out: {operation}')
                result = json.loads(process.stdout.readline())
                self.assertTrue(result['ok'], result)
                return result['simulation']

            try:
                initial = admin('advance 20')
                baseline = admin('direct_events state')['eventResult']
                self.assertEqual((baseline['buttonCount'], baseline['held'], baseline['encoderButtons']), (1, [False], []))
                self.assertEqual(baseline['head'], 0)
                descriptor = admin('direct_action describe 0')['actionResult']
                self.assertEqual([a['id'] for a in descriptor['actions']], ['led.status.set_mode'])
                self.assertEqual(admin('direct_action invoke 1 led.status.set_mode on')['actionResult']['values'], ['on'])
                # Both edges occur before any event read, while HID has no owner.
                admin('contact 0 1'); admin('contact 0 0')
                result = admin('direct_events read 1 0')['eventResult']
                self.assertEqual(result['events'], [{'sequence': 1, 'control': 0, 'pressed': True},
                                                  {'sequence': 2, 'control': 0, 'pressed': False}])
                repeated = admin('direct_events read 1 0')['eventResult']
                for field in ('stream', 'after', 'next', 'events'):
                    self.assertEqual(repeated[field], result[field])
                admin('direct 1'); admin('contact 0 1'); admin('contact 0 0'); admin('direct 0')
                latest = admin('inspect')
                self.assertEqual((latest['events']['stream'], latest['events']['head']), (1, 4))
                for field in ('settingsErases', 'settingsPrograms', 'moduleReconnects', 'descriptorHex'):
                    self.assertEqual(latest[field], initial[field], field)
                for _ in range(40):
                    admin('contact 0 1'); admin('contact 0 0')
                overflow = admin('direct_events read 1 0')['eventResult']
                self.assertEqual(overflow['error'], 'eventGap')
                state = admin('direct_events state')['eventResult']
                self.assertEqual((state['head'], state['oldest'], state['overflow']), (84, 21, 20))
                after = state['oldest'] - 1
                recovered = []
                while True:
                    page = admin(f'direct_events read 1 {after}')['eventResult']
                    self.assertLessEqual(len(page['events']), 16)
                    recovered.extend(page['events']); after = page['next']
                    if not page['more']:
                        break
                self.assertEqual([event['sequence'] for event in recovered], list(range(21, 85)))
                self.assertEqual(admin('direct_events read 1 85')['eventResult']['error'], 'invalidCursor')
                admin('direct_config claim 81'); admin('direct_config name 81 4e6577206d617070696e67')
                changed = admin('direct_config apply 81')
                deadline = time.monotonic() + 4
                while changed['module']['pendingApply'] and time.monotonic() < deadline:
                    changed = admin('inspect')
                self.assertFalse(changed['module']['pendingApply'])
                self.assertEqual(changed['events']['stream'], 2)
                self.assertEqual(admin('direct_events read 1 84')['eventResult']['error'], 'staleStream')
                rebased = admin('direct_events state')['eventResult']
                self.assertEqual((rebased['head'], rebased['held']), (0, [False]))
                rebooted = admin('reboot')
                self.assertGreater(rebooted['moduleBoot'], initial['moduleBoot'])
                self.assertEqual((rebooted['events']['stream'], rebooted['events']['head']), (1, 0))
                self.assertEqual(rebooted['actions']['states'][0]['values'], ['auto'])
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(4)
                errors = process.stderr.read()
                self.assertNotIn('AddressSanitizer', errors)
                self.assertNotIn('runtime error:', errors)
                for stream in (process.stdin, process.stdout, process.stderr):
                    stream.close()

    def test_radio_ports_encoder_switches_and_calibrated_axis_without_hid(self):
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            command = [sys.executable, str(REPO / 'simulation/run.py'), '--config',
                       str(REPO / 'simulation/configs/radio-panel.json'), '--output-dir', str(BINARY_DIR),
                       '--no-build', '--state-dir', str(Path(temporary) / 'state'),
                       '--gateway', '8ea15ffe17734f2b81a12a12']
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, bufsize=1, env=self.environment)
            request = 0

            def admin(operation):
                nonlocal request
                request += 1
                process.stdin.write(f'sim {request} {operation}\n'); process.stdin.flush()
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    self.assertTrue(selector.select(4), f'timed out: {operation}')
                result = json.loads(process.stdout.readline())
                self.assertTrue(result['ok'], result)
                return result['simulation']

            def write_display(index, text):
                ids = ('display.uhf.frequency.set_text', 'display.uhf.preset.set_text', 'display.text.set_text')
                return admin(f'direct_action invoke 1 {ids[index]} {text.encode().hex() or "-"}')

            try:
                initial = admin('advance 20')
                baseline = admin('direct_events state')['eventResult']
                self.assertEqual((baseline['buttonCount'], baseline['contactButtonCount'], baseline['maintainedButtons'], baseline['encoderButtons']),
                                 (7, 3, [0, 1], [3, 4, 5, 6]))
                self.assertEqual((baseline['axisCount'], baseline['axisUsage'], baseline['axisNeutral'], baseline['axes'], baseline['axisAvailable']),
                                 (1, [48], [0], [0], [True]))
                catalogue = admin('direct_action describe 0')['actionResult']['actions']
                self.assertEqual([a['display'] for a in catalogue if 'display' in a],
                                 [{'columns': 7, 'rows': 1, 'encoding': 'ascii'},
                                  {'columns': 2, 'rows': 1, 'encoding': 'ascii'},
                                  {'columns': 16, 'rows': 1, 'encoding': 'utf8'}])
                written = write_display(0, '251.000')
                self.assertEqual(written['displays'][0]['text'], '251.000')
                self.assertEqual(written['displays'][0]['writes'], 1)
                self.assertFalse(write_display(0, '251.0000')['actionResult']['ok'])
                self.assertFalse(write_display(0, 'é')['actionResult']['ok'])
                self.assertEqual(admin('inspect')['displays'][0]['writes'], 1)
                self.assertEqual(write_display(1, '20')['actionResult']['values'], ['20'])
                quoted = 'é "波" \\😀'
                self.assertEqual(write_display(2, quoted)['displays'][2]['text'], quoted)
                boundary = '😀' * 15 + '波'  # exactly 63 UTF-8 bytes / 16 cells
                self.assertEqual(write_display(2, boundary)['actionResult']['values'], [boundary])
                self.assertFalse(write_display(2, '😀' * 16)['actionResult']['ok'])
                self.assertFalse(write_display(2, 'x' * 17)['actionResult']['ok'])
                admin('display_fail 0 1'); failed = write_display(0, '123.450')
                self.assertEqual(failed['actionResult']['error'], 'failed')
                self.assertEqual(failed['displays'][0]['text'], '251.000')
                admin('display_fail 0 0'); admin('display_read_fail 1 1')
                uncertain = write_display(1, '01')
                self.assertEqual(uncertain['actionResult']['error'], 'failed')
                self.assertEqual(uncertain['displays'][1]['text'], '01')
                admin('display_read_fail 1 0')
                self.assertEqual(admin('direct_action state')['actions']['states'][2]['values'], ['01'])
                # Maintained contacts and both detent directions use the same
                # mapped log while the product HID report remains inactive.
                admin('contact 0 1'); admin('contact 1 1'); admin('contact 2 1'); admin('contact 2 0')
                for pin, pressed in ((3, 1), (4, 1), (3, 0), (4, 0), (6, 1), (5, 1), (6, 0), (5, 0)):
                    admin(f'contact {pin} {pressed}')
                events = admin('direct_events read 1 0')['eventResult']
                self.assertEqual([(e['control'], e['pressed']) for e in events['events']],
                                 [(0, True), (1, True), (2, True), (2, False), (3, True), (3, False), (6, True), (6, False)])
                self.assertEqual(events['held'][:3], [True, True, False])
                # Rapid turns traverse sampled quadrature at 2 ms per edge,
                # independently of HID's 25 ms pulse/ack pacing.
                admin('turn 0 3'); admin('turn 1 -2')
                rapid = admin('direct_events read 1 8')['eventResult']
                self.assertEqual([(e['control'], e['pressed']) for e in rapid['events']],
                                 [(3, True), (3, False)] * 3 + [(6, True), (6, False)] * 2)
                admin('display_delay 0 15')
                started = time.monotonic()
                delayed = write_display(0, '123.450')
                self.assertGreaterEqual(time.monotonic() - started, .015)
                self.assertEqual(delayed['displays'][0]['text'], '123.450')
                self.assertEqual(delayed['displays'][0]['delayMs'], 15)
                admin('display_delay 0 0')
                self.assertEqual(admin('axis 0 3900')['events']['axes'], [65535])
                untouched = admin('inspect')
                for field in ('settingsErases', 'settingsPrograms', 'moduleReconnects', 'descriptorHex'):
                    self.assertEqual(untouched[field], initial[field], field)
                admin('contact 0 0'); admin('contact 1 0')
                admin('direct_config claim 7')
                admin('direct_config axis 7 0 0 48 100 3900 32768 1 1000')
                applied = admin('direct_config apply 7'); deadline = time.monotonic() + 4
                while applied['module']['pendingApply'] or applied['module']['generation'] != 2:
                    self.assertLess(time.monotonic(), deadline, applied)
                    applied = admin('inspect')
                self.assertEqual(admin('direct_events read 1 8')['eventResult']['error'], 'staleStream')
                self.assertEqual(admin('axis 0 2000')['events']['axes'], [32768])
                extreme = admin('axis 0 100')
                self.assertEqual(extreme['events']['axes'], [65535])
                self.assertEqual(extreme['events']['axisNeutral'], [32768])
                admin('direct 1'); self.assertEqual(admin('inspect')['moduleReportHex'], '00ffff')
                admin('direct 0')
                stale = admin('axes_available 0')['events']
                self.assertEqual((stale['axisAvailable'], stale['axes']), ([False], [32768]))
                self.assertEqual(admin('axes_available 1')['events']['axisAvailable'], [True])
                rebooted = admin('reboot')
                self.assertEqual([d['text'] for d in rebooted['displays']], ['', '', ''])
                self.assertEqual(rebooted['events']['axisNeutral'], [0])
            finally:
                process.terminate(); process.wait(4)
                errors = process.stderr.read()
                self.assertNotIn('AddressSanitizer', errors); self.assertNotIn('runtime error:', errors)
                for stream in (process.stdin, process.stdout, process.stderr):
                    stream.close()

    def test_actions_are_volatile_and_independent_of_input_configuration(self):
        with tempfile.TemporaryDirectory(dir=BINARY_DIR) as temporary:
            directory = Path(temporary)
            command = [sys.executable, str(REPO / 'simulation/run.py'), '--config',
                       str(REPO / 'simulation/configs/pedals.json'), '--output-dir', str(BINARY_DIR),
                       '--no-build', '--state-dir', str(directory / 'state'),
                       '--gateway', '8ea15ffe17734f2b81a12a12']
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, bufsize=1, env=self.environment)
            request = 0

            def admin(operation):
                nonlocal request
                request += 1
                process.stdin.write(f'sim {request} {operation}\n'); process.stdin.flush()
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    self.assertTrue(selector.select(4), f'timed out: {operation}')
                result = json.loads(process.stdout.readline())
                self.assertTrue(result['ok'], result)
                return result['simulation']

            def states(result):
                return {entry['id']: entry['values'] for entry in result['actions']['states']}

            try:
                initial = admin('inspect')
                described = admin('direct_action describe 0')['actionResult']
                self.assertTrue(described['ok'])
                self.assertEqual([entry['id'] for entry in described['actions']],
                                 ['led.rgb.set_color', 'led.boolean.set_enabled'])
                self.assertEqual(states(initial), {'led.rgb.set_color': ['000000'],
                                                  'led.boolean.set_enabled': [False]})
                colored = admin('direct_action invoke 1 led.rgb.set_color 12abef')
                self.assertTrue(colored['actionResult']['ok'])
                self.assertEqual(states(colored)['led.rgb.set_color'], ['12ABEF'])
                enabled = admin('direct_action invoke 1 led.boolean.set_enabled 1')
                self.assertEqual(states(enabled)['led.boolean.set_enabled'], [True])
                for operation, error in (
                    ('invoke 2 led.rgb.set_color 000000', 'staleCatalogue'),
                    ('invoke 1 led.rgb.set_color invalid', 'invalidArgument'),
                    ('invoke 1 led.boolean.set_enabled 2', 'invalidArgument'),
                    ('invoke 1 missing.action 1', 'unknownAction'),
                ):
                    rejected = admin(f'direct_action {operation}')
                    self.assertFalse(rejected['actionResult']['ok'])
                    self.assertEqual(rejected['actionResult']['error'], error)
                    self.assertEqual(states(rejected), states(enabled))
                admin('direct_config claim 71')
                direct = admin('direct 1')
                disabled = admin('direct_action invoke 1 led.boolean.set_enabled 0')
                self.assertTrue(disabled['actionResult']['ok'])
                self.assertEqual(states(disabled)['led.boolean.set_enabled'], [False])
                routed = admin('direct 0')
                self.assertEqual(states(routed), states(disabled))
                for result in (colored, enabled, direct, disabled, routed):
                    for field in ('settingsErases', 'settingsPrograms', 'moduleReconnects', 'descriptorHex'):
                        self.assertEqual(result[field], initial[field], field)
                    self.assertEqual(result['module']['config'], initial['module']['config'])
                rebooted = admin('reboot')
                self.assertGreater(rebooted['moduleBoot'], initial['moduleBoot'])
                self.assertEqual(states(rebooted), states(initial))
                self.assertEqual(rebooted['settingsErases'], initial['settingsErases'])
                self.assertEqual(rebooted['settingsPrograms'], initial['settingsPrograms'])
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(4)
                errors = process.stderr.read()
                self.assertNotIn('AddressSanitizer', errors)
                self.assertNotIn('runtime error:', errors)
                for stream in (process.stdin, process.stdout, process.stderr):
                    stream.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=BINARY_DIR)
    parser.add_argument('--common-source', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    BINARY_DIR = args.output_dir.resolve()
    build(BINARY_DIR, args.common_source, args.sanitize)
    unittest.main(argv=[sys.argv[0]], verbosity=2)
