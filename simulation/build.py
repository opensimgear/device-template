#!/usr/bin/env python3
"""Build the native device from this repository and its pinned firmware-common."""
import argparse
import hashlib
import fcntl
import json
from pathlib import Path
import re
import shutil
import subprocess

REPO = Path(__file__).resolve().parents[1]


def build(output_dir, common_source=None, sanitize=False):
    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    # Several independently launched devices may initialize the same build.
    with (output_dir / '.build.lock').open('a+') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        return _build(output_dir, common_source, sanitize)


def _build(output_dir, common_source, sanitize):
    firmware = REPO / 'firmware'
    revision = re.search(r'OSG_COMMON_REVISION\s+([0-9a-f]{40})',
                         (firmware / 'cmake/common-revision.cmake').read_text()).group(1)
    common = Path(common_source).resolve() if common_source else output_dir / 'deps/firmware-common'
    if not common_source and not common.exists():
        common.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['git', 'clone', '--no-checkout', 'https://github.com/opensimgear/firmware-common.git', str(common)], check=True)
        subprocess.run(['git', '-C', str(common), 'checkout', '--detach', revision], check=True)
        subprocess.run(['git', '-C', str(common), 'submodule', 'update', '--init', '--recursive'], check=True)
    if not common_source:
        actual = subprocess.check_output(['git', '-C', str(common), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != revision:
            raise ValueError(f'{common}: expected firmware-common revision {revision}, found {actual}')
    cyphal = firmware / 'shared-cyphal'
    sources = [Path(__file__).with_name('device.c'),
               *[firmware / 'src' / name for name in ('can_client.c', 'can_delivery.c', 'runtime_config.c', 'config_can.c', 'boot_session.c')],
               firmware / 'shared-gateway/src/gateway_journal.c',
               *[common / 'src/inputs' / name for name in ('profile.c', 'inputs.c', 'button.c')], common / 'src/config/line.c',
               cyphal / 'src/cyphal.c', cyphal / 'src/gateway_protocol.c', cyphal / 'vendor/libcanard/canard.c']
    includes = [firmware / 'include', firmware / 'shared-gateway/include', common / 'include',
                cyphal / 'include', cyphal / 'generated', cyphal / 'vendor/libcanard']
    for path in sources:
        if not path.is_file():
            raise ValueError(f'Required source missing: {path}')
    compiler = shutil.which('cc')
    if not compiler:
        raise ValueError('A C11 compiler (cc) is required')
    flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG']
    if sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    output = output_dir / 'osg-device-sim'
    command = [compiler, *flags, *[arg for path in includes for arg in ('-I', str(path))], *map(str, sources), '-o', str(output)]
    digest = hashlib.sha256(json.dumps(command).encode())
    for path in [*sources, *sorted({p for directory in includes for p in directory.rglob('*.h')})]:
        digest.update(str(path).encode()); digest.update(path.read_bytes())
    fingerprint = output_dir / 'build.sha256'
    if not output.is_file() or not fingerprint.is_file() or fingerprint.read_text() != digest.hexdigest():
        subprocess.run(command, check=True)
        fingerprint.write_text(digest.hexdigest())
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=REPO / '.simulation')
    parser.add_argument('--common-source', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    try:
        print(build(args.output_dir, args.common_source, args.sanitize))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Build failed: {error}\n')


if __name__ == '__main__':
    main()
