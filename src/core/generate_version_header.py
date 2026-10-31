# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Configure the version header using declared inputs and hermetic Python."""
import argparse
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('template', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('module_revision')
    parser.add_argument('--workspace-status', type=Path)
    args = parser.parse_args()
    revision = args.module_revision
    if args.workspace_status:
        entries = dict(line.split(' ', 1) for line in args.workspace_status.read_text().splitlines() if ' ' in line)
        revision = entries.get('STABLE_NAJA_GIT_HASH', 'unknown')
    if revision != 'unknown' and not re.fullmatch('[0-9a-fA-F]+', revision):
        raise ValueError('Invalid Naja producer revision')
    args.output.write_bytes(args.template.read_bytes().replace(b'@NAJA_GIT_HASH@', revision.encode('ascii')))


if __name__ == '__main__':
    main()
