#!/usr/bin/env python3
"""Exercise JSON startup, production configuration and flash restart behavior."""
import argparse
import copy
import json
from pathlib import Path
import selectors
import subprocess
import sys
import tempfile
import unittest
sys.dont_write_bytecode = True
from build import build, REPO
from configuration import ConfigurationError, encode_configuration, read_configuration

BINARY_DIR = REPO / '.simulation'


class ConfigurationTests(unittest.TestCase):
    def setUp(self):
        self.pedals = json.loads((REPO / 'simulation/configs/pedals.json').read_text())

    def test_examples_pass_native_validation(self):
        for name in ('pedals', 'button-panel', 'shifter'):
            result = subprocess.run([sys.executable, str(REPO / 'simulation/run.py'), '--config',
                                     str(REPO / f'simulation/configs/{name}.json'), '--output-dir', str(BINARY_DIR),
                                     '--no-build', '--validate'], capture_output=True, text=True)
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
                                        stderr=subprocess.PIPE, text=True, bufsize=1)

            def admin(operation):
                nonlocal request
                request += 1
                process.stdin.write(f'sim {request} {operation}\n'); process.stdin.flush()
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    self.assertTrue(selector.select(4), f'timed out: {operation}')
                line = process.stdout.readline()
                if not line:
                    self.fail(f'process exited {process.poll()}: {process.stderr.read()}')
                result = json.loads(line)
                self.assertTrue(result['ok'], result)
                return result['simulation']

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
                saved = admin('direct_config apply_save 1')
                self.assertIn('Saved pedals', json.dumps(saved['module']))
                stop(kill=True)
                # Altered JSON remains a reset target; committed settings win.
                self.pedals['name'] = 'Changed JSON defaults'
                config.write_text(json.dumps(self.pedals))
                process = start()
                restored = admin('inspect')
                self.assertGreater(restored['moduleBoot'], first['moduleBoot'])
                self.assertIn('Saved pedals', json.dumps(restored['module']))
                duplicate = subprocess.run(command, input='', capture_output=True, text=True, timeout=4)
                self.assertNotEqual(duplicate.returncode, 0)
                self.assertIn('already in use', duplicate.stderr)
                admin('direct 1')
                admin('direct_config claim 2')
                admin('direct_config reset 2')
                reset = admin('direct_config apply_save 2')
                self.assertIn('Changed JSON defaults', json.dumps(reset['module']))
            finally:
                if process is not None and not process.stdout.closed:
                    stop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=BINARY_DIR)
    parser.add_argument('--common-source', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    BINARY_DIR = args.output_dir.resolve()
    build(BINARY_DIR, args.common_source, args.sanitize)
    unittest.main(argv=[sys.argv[0]], verbosity=2)
