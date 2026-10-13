#!/usr/bin/env python3
"""Launch one device from JSON, preserving stdin, signals and the native process ID."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
sys.dont_write_bytecode = True
from build import build, REPO
from configuration import read_configuration


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--gateway', default='00112233445566778899aabb')
    parser.add_argument('--state-dir', type=Path)
    parser.add_argument('--output-dir', type=Path, default=REPO / '.simulation')
    parser.add_argument('--common-source', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--no-build', action='store_true', help='use the executable already in --output-dir')
    parser.add_argument('--validate', action='store_true', help='validate JSON and firmware profile, then exit')
    args = parser.parse_args()
    if not re.fullmatch('[0-9a-f]{24}', args.gateway) or args.gateway in ('0' * 24, 'f' * 24):
        parser.error('--gateway must be a nonzero, non-ff lowercase 24-character hexadecimal identity')
    try:
        identity, payload = read_configuration(args.config)
        output = args.output_dir.resolve()
        binary = output / 'osg-device-sim' if args.no_build else build(output, args.common_source, args.sanitize)
        state = (args.state_dir or output / 'state' / identity).resolve()
        # One bounded pipe transports the validated startup definition without
        # scratch configuration files or environment variables. Native C repeats
        # profile validation before flash writes, sockets or CAN announcements.
        read_fd, write_fd = os.pipe()
        os.set_inheritable(read_fd, True)
        try:
            os.write(write_fd, payload)
        finally:
            os.close(write_fd)
        command = [str(binary), '--definition-fd', str(read_fd), '--gateway', args.gateway, '--state-dir', str(state)]
        if args.validate:
            command.append('--validate')
        os.execv(binary, command)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(2, f'Device startup failed: {error}\n')


if __name__ == '__main__':
    main()
