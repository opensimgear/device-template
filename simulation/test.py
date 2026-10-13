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
        for name in ('pedals', 'button-panel', 'shifter'):
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
