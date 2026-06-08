#!/usr/bin/env python3
"""Script wrapper for running the XY SecBoot CLI from the repo checkout."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from xy_secboot.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
