#!/usr/bin/env python3
"""
Download pre-built Comet release binaries from GitHub for regression testing.

For each tag listed in BASELINE_TAGS, fetches the appropriate binary from
the Comet GitHub Releases page and places it under:
  baselines/<tag>/Comet.exe   (Windows)
  baselines/<tag>/comet       (Linux)

Usage:
    python setup_baselines.py
    python setup_baselines.py --tags v2026.01.1 v2025.01.0
    python setup_baselines.py --list     # show tags and presence status

Asset naming on https://github.com/UWPR/Comet/releases:
    Windows : comet.win64.exe
    Linux   : comet.linux.exe
"""

import argparse
import os
import platform
import stat
import sys
import urllib.request
from pathlib import Path

REPO            = "UWPR/Comet"
GITHUB_DL_BASE  = f"https://github.com/{REPO}/releases/download"
BASELINES_DIR   = Path(__file__).parent / "baselines"

# Tags to fetch by default; add new releases here as they are published.
BASELINE_TAGS = [
    "v2026.01.1",
]

IS_WINDOWS = (platform.system() == "Windows") or (os.name == "nt")


def asset_url(tag: str) -> tuple[str, str]:
    """Return (download_url, local_filename) for this platform."""
    if IS_WINDOWS:
        return f"{GITHUB_DL_BASE}/{tag}/comet.win64.exe", "Comet.exe"
    else:
        return f"{GITHUB_DL_BASE}/{tag}/comet.linux.exe", "comet"


def setup_tag(tag: str) -> bool:
    url, local_name = asset_url(tag)
    tag_dir = BASELINES_DIR / tag
    dest    = tag_dir / local_name

    if dest.exists():
        print(f"{tag}: already present at {dest}")
        return True

    print(f"{tag}: downloading {url}")
    tag_dir.mkdir(parents=True, exist_ok=True)
    try:
        urllib.request.urlretrieve(url, dest)
    except Exception as e:
        print(f"  ERROR: {e}", file=sys.stderr)
        return False

    if not IS_WINDOWS:
        dest.chmod(dest.stat().st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)

    print(f"  saved -> {dest}")
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tags", nargs="+", default=BASELINE_TAGS,
                        help="release tags to download (default: BASELINE_TAGS list)")
    parser.add_argument("--list", action="store_true",
                        help="list configured tags and exit")
    args = parser.parse_args()

    if args.list:
        for tag in BASELINE_TAGS:
            _, local_name = asset_url(tag)
            dest   = BASELINES_DIR / tag / local_name
            status = "present" if dest.exists() else "missing"
            print(f"  {tag:20s}  {status}  ({dest})")
        return

    BASELINES_DIR.mkdir(parents=True, exist_ok=True)
    results = {tag: setup_tag(tag) for tag in args.tags}
    failed  = [t for t, ok in results.items() if not ok]
    if failed:
        print(f"\nFailed: {failed}", file=sys.stderr)
        sys.exit(1)
    print("\nAll baselines ready.")


if __name__ == "__main__":
    main()
