#!/usr/bin/env python3
"""
Umbrella CLI for the Carafe ahead-of-time pipeline (docs/20260826_carafe.md): one entry
point, one subcommand per stage, identical on WSL Ubuntu and native Windows
(`./tools/carafe.py ...` or `python tools\\carafe.py ...`).

    tools/carafe.py prerun --fasta db.fasta --out DIR --comet PATH --flavor n=f.params
    tools/carafe.py prerun --fasta db.fasta --out DIR --comet PATH --params comet.params
    tools/carafe.py <command> --help          full options for any stage

Each subcommand is a thin dispatch to the module that implements it (listed below);
the modules remain directly runnable (`python3 tools/carafe_prerun.py ...`) and their
docstrings carry the full per-stage documentation. Imports are lazy so stages with
heavy dependencies (numpy, the Carafe venv) don't tax the others.
"""

import importlib
import os
import sys

# command -> (module under tools/, one-line description). Order = help order =
# pipeline order, with the legacy/aux tools at the end.
COMMANDS = {
    "prerun":      ("carafe_prerun",
                    "full pipeline driver: .idx build -> variant export -> convert -> "
                    "predict -> .cps -> .fi_mask (resumable; --params for the "
                    "single-flavor case)"),
    "convert":     ("idx_to_carafe",
                    "convert a .idx + comet.exe -x variant export into Carafe "
                    "ai_pred.py input TSV (+ variant-map sidecar)"),
    "predict":     ("run_carafe_chunked",
                    "run Carafe ai_pred.py over an input TSV in resumable chunks "
                    "(the expensive step)"),
    "cps":         ("carafe_pred_to_cps",
                    "translate a chunked prediction run into a compact prediction "
                    "store (.cps, the durable artifact)"),
    "mask":        ("carafe_cps_to_fi_mask",
                    "build/re-sweep a .fi_mask from a .cps store (the normal mask "
                    "path)"),
    "inten":       ("carafe_cps_to_inten",
                    "build a .carafe_inten predicted-intensity file from a .cps store "
                    "(input for the intensity score)"),
    "mask-tsv":    ("carafe_ms2_to_fi_mask",
                    "build a .fi_mask directly from ms2_df/ms2_pred TSVs (tests, "
                    "small runs)"),
    "mask-chunks": ("build_carafe_mask_chunked",
                    "legacy chunked-TSV mask build, one carafe_ms2_to_fi_mask.py run "
                    "per prediction chunk"),
    "merge-masks": ("merge_carafe_fi_masks",
                    "merge per-chunk .fi_mask files (mask-chunks output) into one "
                    "final mask"),
}


def usage(out=sys.stdout):
    prog = os.path.basename(sys.argv[0])
    print(__doc__.strip().split("\n\n")[0], file=out)
    print(f"\nUsage: {prog} <command> [options]   "
          f"({prog} <command> --help for a stage's options)\n", file=out)
    width = max(len(c) for c in COMMANDS)
    for cmd, (mod, desc) in COMMANDS.items():
        print(f"  {cmd:<{width}}  {desc}", file=out)
        print(f"  {'':<{width}}  [tools/{mod}.py]", file=out)


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        usage()
        sys.exit(0 if len(sys.argv) >= 2 else 2)
    cmd = sys.argv[1]
    if cmd not in COMMANDS:
        print(f"Unknown command: {cmd!r}\n", file=sys.stderr)
        usage(out=sys.stderr)
        sys.exit(2)
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    module = importlib.import_module(COMMANDS[cmd][0])
    module.main(sys.argv[2:])


if __name__ == "__main__":
    main()
