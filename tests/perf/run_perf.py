#!/usr/bin/env python3
"""
Comet performance benchmarks -- wall-clock time and peak memory across search modes.

Runs the current build (and optionally baseline binaries) in three search modes:
  1. Regular FASTA search
  2. Fragment ion index search   (requires pre-built .idx)
  3. Peptide index search        (requires pre-built .idx)

Metrics collected per run:
  - Wall-clock time (seconds)
  - Peak RSS memory (MB)        via /usr/bin/time -v on Linux, tasklist on Windows

Output written to reports/<timestamp>.json and a human-readable text summary.

Usage:
    python run_perf.py
    python run_perf.py --comet ../../x64/Release/Comet.exe
    python run_perf.py --baseline ../regression/baselines/v2024.01.0/Comet.exe
    python run_perf.py --runs 3        # repeat each mode N times and take median

TODO: implement search invocation, timing, memory collection, and report writing.
"""

import argparse
import sys
from pathlib import Path

PERF_DIR   = Path(__file__).parent
REPORTS_DIR = PERF_DIR / "reports"
REPO_ROOT  = PERF_DIR.parent.parent
DATA_DIR   = REPO_ROOT / "data"
COMET_EXE  = REPO_ROOT / "x64" / "Release" / "Comet.exe"


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--comet",    type=Path, default=COMET_EXE,
                        help=f"Comet binary to benchmark (default: {COMET_EXE})")
    parser.add_argument("--baseline", type=Path,
                        help="optional second binary to compare against")
    parser.add_argument("--data",     type=Path, default=DATA_DIR,
                        help=f"directory with .mzXML/.raw and .params files (default: {DATA_DIR})")
    parser.add_argument("--runs",     type=int, default=1,
                        help="number of timed repetitions per mode (default: 1)")
    args = parser.parse_args()

    # TODO: implement
    print("run_perf.py: not yet implemented")
    print(f"  binary  : {args.comet}")
    print(f"  data    : {args.data}")
    print(f"  runs    : {args.runs}")
    sys.exit(0)


if __name__ == "__main__":
    main()
