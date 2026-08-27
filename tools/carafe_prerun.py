#!/usr/bin/env python3
"""
The Carafe ahead-of-time pipeline driver -- docs/20260826_carafe.md milestone M4.
Runs everything that must happen BEFORE a masked Comet FI search, end to end and
resumable, so search time only ever consumes a finished .fi_mask file (never triggers
Carafe, ai_pred.py, or any Python):

  per flavor:  [s1] comet.exe -i  .idx build      (each flavor gets its own FASTA copy,
                                                 since Comet writes <database_name>.idx
                                                 -- two flavors of one FASTA would clobber)
               [s2] comet.exe -x  variant export
               [s3] idx_to_carafe.py              (out_tsv + variant-map sidecar)
  once:        [s4] run_carafe_chunked.py         (Carafe ai_pred.py -- the expensive step,
                                                 hours; chunked + resumable on its own)
               [s5] carafe_pred_to_cps.py         (compact prediction store; raw TSVs
                                                 become deletable -- see --delete-raw)
  per flavor:  [s6] carafe_cps_to_fi_mask.py      (~tens of minutes; --ignore-modloss is
                                                 AUTO-DETECTED per flavor from its own
                                                 VarModConfig: all neutral-loss deltas
                                                 0.0 -> general mode)

(Python port of the original tools/carafe_prerun.sh, so the pipeline also runs in a
native Windows terminal. Stage names, marker files, and log layout are identical to the
bash era's, so a workdir half-finished under the old driver resumes under this one.
Normally invoked through the umbrella CLI: `tools/carafe.py prerun ...`.)

"Flavor" = one comet.params mod configuration sharing the same peptide population --
canonically a withNL/noNL pair (same mods, neutral_loss zeroed in one). The FIRST flavor
listed is the PRIMARY: its out_tsv feeds Carafe and the store. Every other flavor's
conversion must produce the identical row count (checked, loud failure otherwise) --
guaranteeing the store's row_index space is valid for all flavors' variant maps.

Resume: each stage writes OUT/.prerun/<stage>.done on success and is skipped when that
marker exists; delete a marker to re-run its stage (stage 4 additionally resumes at
chunk granularity via run_carafe_chunked.py's own markers). --stop-after lets a partial
pipeline end deliberately (e.g. everything up to prediction on a GPU machine).

Usage:
  tools/carafe.py prerun --fasta FILE --out DIR --comet PATH \\
      --flavor NAME=PARAMS_FILE [--flavor NAME=PARAMS_FILE ...] [options]
  tools/carafe.py prerun --fasta FILE --out DIR --comet PATH --params comet.params

Options:
  --fasta FILE            protein database (target+decoy FASTA)
  --out DIR               working/output directory (created if missing)
  --comet PATH            comet.exe to build/export with (full path)
  --flavor NAME=FILE      repeatable; first = primary. NAME becomes the file prefix
                          (NAME.fasta.idx, NAME.carafe_peptides.tsv, NAME.fi_mask, ...)
  --params FILE           single-flavor shorthand (formerly params_to_fi_mask.sh):
                          equivalent to --flavor primary=FILE, plus the mask thresholds
                          are read from the file's carafe_mask_min_relative_intensity /
                          carafe_mask_min_peaks keys when present. An explicit
                          --min-relative-intensity/--min-kept-peaks flag always wins.
                          Mutually exclusive with --flavor.
  --charges LIST          idx_to_carafe.py --charges (default: its own default "2,3";
                          the validated production run used "2")
  --include-decoys        pass through to idx_to_carafe.py (production runs did)
  --carafe-mode MODE      ai_pred.py --mode (default: phosphorylation)
  --parquet               transient parquet mode for stages 4-5 (prediction output
                          ~5-9x smaller; stores verified byte-identical to the TSV
                          path -- see run_carafe_chunked.py's --parquet). Runs the
                          s5 translator under the Carafe venv python (pandas/pyarrow).
  --chunk-size N          run_carafe_chunked.py chunk size (default 50000)
  --quant u8|u16          store quantization (default u16 -- the validated choice;
                          u8 diverges on ~2.8% of variants, see plan doc Section 5.4)
  --min-relative-intensity F / --min-kept-peaks N   mask thresholds (defaults 0.10 / 6)
  --workers N             parallelism for translation + mask builds (default: cpus-2)
  --venv-python PATH / --ai-pred-py PATH   forwarded to run_carafe_chunked.py
  --stop-after STAGE      stop after: idx, export, convert, predict, cps, mask
  --delete-raw            after the store verifies (s5), delete the raw per-chunk Carafe
                          prediction output (the ~hundreds-of-GB transient). Default OFF.

Every stage's stdout/stderr lands in OUT/.prerun/<stage>.log.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import carafe_chunk_common as common  # noqa: E402

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------------------
# Helpers (module-level so tests/unit/test_carafe_pipeline_drivers.py can import them)
# ---------------------------------------------------------------------------

def params_with(src, dst, key, value):
    """Rewrite one param in a params file copy (whole-line replacement, key must
    exist). Line endings of the source file are preserved byte-for-byte -- comet.params
    files in this repo are CRLF and must stay that way."""
    key_re = re.compile(r"^" + re.escape(key) + r"\s*=")
    found = False
    out_lines = []
    with open(src, "r", newline="") as f:
        for raw in f:
            body = raw.rstrip("\r\n")
            eol = raw[len(body):]
            if key_re.match(body):
                body = f"{key} = {value}"
                found = True
            out_lines.append(body + eol)
    if not found:
        raise ValueError(f"params file {src} has no '{key}' line -- add one "
                         f"(e.g. '{key} =') first")
    with open(dst, "w", newline="") as f:
        f.writelines(out_lines)


def get_param(params_path, key):
    """Value of the first 'key = value' line in a comet.params file (up to the next
    whitespace or '#' comment), or None if the key is absent."""
    pat = re.compile(r"^" + re.escape(key) + r"\s*=\s*([^\s#]+)")
    with open(params_path, errors="replace") as f:
        for line in f:
            m = pat.match(line)
            if m:
                return m.group(1)
    return None


def parse_rows_written(log_text):
    """First 'Wrote N rows' count in an idx_to_carafe.py log, or None."""
    m = re.search(r"Wrote (\d+) rows", log_text)
    return int(m.group(1)) if m else None


def varmodconfig_has_nl(vmc_line):
    """True if a variant-map '# VarModConfig: ...' line carries any nonzero
    neutral-loss delta. Fields look like '79.966331STY--97.976896|...'; the number
    after each '--' is that slot's NL delta. Same regex the bash driver used: a '--'
    followed by a number with a nonzero integer part, or a zero integer part and a
    nonzero fraction."""
    return re.search(r"--(0*[1-9][0-9]*\.|0*\.0*[1-9])", vmc_line) is not None


class PrerunDriver:
    def __init__(self, args):
        self.args = args
        self.state = os.path.join(args.out_dir, ".prerun")
        os.makedirs(self.state, exist_ok=True)
        self.flavor_names = [name for name, _ in args.flavors]
        self.flavor_params = dict(args.flavors)
        self.primary = self.flavor_names[0]

    def stage_done(self, stage):
        return os.path.isfile(os.path.join(self.state, f"{stage}.done"))

    def mark_done(self, stage):
        with open(os.path.join(self.state, f"{stage}.done"), "w", newline="\n"):
            pass

    def maybe_stop(self, kind):
        """Call after finishing stage-kind `kind` (idx/export/convert/predict/cps/mask)."""
        if self.args.stop_after == kind:
            print(f"[driver] --stop-after {kind} reached; stopping.")
            sys.exit(0)

    def run_stage(self, stage, desc, cmd):
        if self.stage_done(stage):
            print(f"[{stage}] already done, skipping")
            return
        print(f"[{stage}] {desc} ...")
        t0 = time.monotonic()
        log_path = os.path.join(self.state, f"{stage}.log")
        with open(log_path, "wb") as log:
            rc = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT,
                                check=False).returncode
        elapsed = int(round(time.monotonic() - t0))
        if rc == 0:
            self.mark_done(stage)
            print(f"[{stage}] done in {elapsed}s")
        else:
            sys.exit(f"[{stage}] FAILED after {elapsed}s -- see {log_path}")

    def stage_log_text(self, stage):
        try:
            with open(os.path.join(self.state, f"{stage}.log"),
                      errors="replace") as f:
                return f.read()
        except OSError:
            return ""

    # ---- stages 1-3, per flavor ----
    def run_idx_export_convert(self):
        a = self.args
        for name in self.flavor_names:
            pf = self.flavor_params[name]
            ffasta = os.path.join(a.out_dir, f"{name}.fasta")
            fidx = ffasta + ".idx"

            if not self.stage_done(f"s1_idx_{name}"):
                shutil.copyfile(a.fasta, ffasta)
                params_with(pf, os.path.join(a.out_dir, f"{name}.build.params"),
                            "database_name", ffasta)
            self.run_stage(f"s1_idx_{name}", f"build {fidx} (comet -i)",
                           [a.comet, "-P" + os.path.join(a.out_dir, f"{name}.build.params"),
                            "-i"])

            if not self.stage_done(f"s2_export_{name}"):
                params_with(pf, os.path.join(a.out_dir, f"{name}.export.params"),
                            "database_name", fidx)
            export_tsv = os.path.join(a.out_dir, f"{name}.variants_export.tsv")
            self.run_stage(f"s2_export_{name}", "export variant enumeration (comet -x)",
                           [a.comet, "-P" + os.path.join(a.out_dir, f"{name}.export.params"),
                            "-x" + export_tsv])

            convert_cmd = [sys.executable, os.path.join(SCRIPT_DIR, "idx_to_carafe.py"),
                           fidx, export_tsv,
                           os.path.join(a.out_dir, f"{name}.carafe_peptides.tsv")]
            if a.charges:
                convert_cmd += ["--charges", a.charges]
            if a.include_decoys:
                convert_cmd += ["--include-decoys"]
            self.run_stage(f"s3_convert_{name}", "idx_to_carafe.py conversion",
                           convert_cmd)
        self.maybe_stop("idx")
        self.maybe_stop("export")

    # Cross-flavor population identity: every flavor's conversion must report the same
    # row count as the primary's (the store's row_index space must be valid for all
    # variant maps).
    def check_population_identity(self):
        primary_rows = parse_rows_written(self.stage_log_text(f"s3_convert_{self.primary}"))
        if primary_rows is None:
            sys.exit(f"cannot read primary row count from s3_convert_{self.primary}.log")
        for name in self.flavor_names:
            rows = parse_rows_written(self.stage_log_text(f"s3_convert_{name}"))
            if rows != primary_rows:
                sys.exit(f"FLAVOR POPULATION MISMATCH: {name} converted {rows} rows vs "
                         f"primary {self.primary}'s {primary_rows} -- flavors must share "
                         f"one peptide population (same enzyme/mass/length/mod "
                         f"residues+masses; only neutral-loss deltas may differ).")
        print(f"[driver] population identity OK: {len(self.flavor_names)} flavor(s) x "
              f"{primary_rows} rows")
        self.maybe_stop("convert")

    # ---- stage 4: Carafe prediction (once, primary flavor's out_tsv) ----
    def run_predict(self):
        a = self.args
        predict_cmd = [sys.executable, os.path.join(SCRIPT_DIR, "run_carafe_chunked.py"),
                       "--in", os.path.join(a.out_dir, f"{self.primary}.carafe_peptides.tsv"),
                       "--out", os.path.join(a.out_dir, "prediction"),
                       "--chunk-size", str(a.chunk_size), "--mode", a.carafe_mode,
                       "--device", "cpu", "--tf-type", "ms2",
                       "--limit-chunks", "0", "--jobs", "1"]
        if a.venv_python:
            predict_cmd += ["--venv-python", a.venv_python]
        if a.ai_pred_py:
            predict_cmd += ["--ai-pred-py", a.ai_pred_py]
        if a.parquet:
            predict_cmd += ["--parquet"]
        self.run_stage("s4_predict",
                       "Carafe prediction (run_carafe_chunked.py -- the expensive step)",
                       predict_cmd)
        # run_carafe_chunked.py logs failures and continues; a chunk without .done
        # means failure.
        n_chunks = len(common.list_chunk_tsvs(os.path.join(a.out_dir, "prediction",
                                                           "chunks")))
        n_done = len(glob.glob(os.path.join(a.out_dir, "prediction", "chunk_preds",
                                            "*", ".done")))
        if n_chunks == 0 or n_done != n_chunks:
            try:
                os.remove(os.path.join(self.state, "s4_predict.done"))
            except OSError:
                pass
            sys.exit(f"prediction incomplete: {n_done}/{n_chunks} chunks done -- "
                     f"re-run to resume")
        self.maybe_stop("predict")

    # ---- stage 5: compact prediction store ----
    def run_cps(self):
        a = self.args
        cps_py = sys.executable
        if a.parquet:
            cps_py = a.venv_python or common.default_venv_python()
        self.run_stage("s5_cps", f"translate predictions -> {self.primary}.cps",
                       [cps_py, os.path.join(SCRIPT_DIR, "carafe_pred_to_cps.py"),
                        "--chunks-dir", os.path.join(a.out_dir, "prediction", "chunks"),
                        "--preds-dir", os.path.join(a.out_dir, "prediction", "chunk_preds"),
                        "--source-out-tsv",
                        os.path.join(a.out_dir, f"{self.primary}.carafe_peptides.tsv"),
                        "--out", os.path.join(a.out_dir, f"{self.primary}.cps"),
                        "--quant", a.quant, "--workers", str(a.workers)])

        if a.delete_raw and not self.stage_done("s5b_delete_raw"):
            print("[s5b_delete_raw] store verified; deleting raw per-chunk Carafe "
                  "output ...")
            shutil.rmtree(os.path.join(a.out_dir, "prediction", "chunk_preds"),
                          ignore_errors=True)
            self.mark_done("s5b_delete_raw")
        self.maybe_stop("cps")

    # ---- stage 6: masks, per flavor (--ignore-modloss auto-detected) ----
    def run_masks(self):
        a = self.args
        for name in self.flavor_names:
            vmap = os.path.join(a.out_dir, f"{name}.carafe_peptides.variants.tsv")
            with open(vmap, errors="replace") as f:
                vmc = f.readline()
            mask_cmd = [sys.executable, os.path.join(SCRIPT_DIR, "carafe_cps_to_fi_mask.py"),
                        os.path.join(a.out_dir, f"{name}.fasta.idx"), vmap,
                        os.path.join(a.out_dir, f"{self.primary}.cps"),
                        os.path.join(a.out_dir, f"{name}.fi_mask"),
                        "--min-relative-intensity", str(a.min_relative_intensity),
                        "--min-kept-peaks", str(a.min_kept_peaks),
                        "--workers", str(a.workers),
                        "--verify-out-tsv",
                        os.path.join(a.out_dir, f"{self.primary}.carafe_peptides.tsv")]
            if not varmodconfig_has_nl(vmc):
                print(f"[driver] flavor {name}: all neutral-loss deltas zero -> "
                      f"--ignore-modloss")
                mask_cmd += ["--ignore-modloss"]
            else:
                print(f"[driver] flavor {name}: neutral-loss delta present -> "
                      f"modloss channels active")
            self.run_stage(f"s6_mask_{name}", f"build {name}.fi_mask from store",
                           mask_cmd)
        self.maybe_stop("mask")

    def run(self):
        self.run_idx_export_convert()
        self.check_population_identity()
        self.run_predict()
        self.run_cps()
        self.run_masks()
        print("[driver] complete. Masks:")
        for name in self.flavor_names:
            mask = os.path.join(self.args.out_dir, f"{name}.fi_mask")
            print(f"  {os.path.getsize(mask):>14,} bytes  {mask}")
        print("[driver] point comet.params fragment_index_predicted_mask_file (or RTS "
              "--mask) at the")
        print("[driver] flavor-matching .fi_mask, with database_name = that flavor's "
              ".fasta.idx.")


def apply_params_shorthand(args):
    """Resolve --params (the single-flavor case, formerly tools/params_to_fi_mask.sh)
    and the mask-threshold defaults on a parsed argument namespace: one comet.params
    becomes the "primary" flavor, and the two Carafe mask-threshold keys are read out
    of it -- an explicit --min-relative-intensity/--min-kept-peaks flag (non-None)
    always wins, then the params keys, then the 0.10/6 defaults. comet.exe accepts and
    stores those keys but never reads them (Comet.cpp's paramHandlers) -- registered
    there purely so the SAME comet.params can also drive the actual masked search
    afterward without a "Warning - invalid parameter found"."""
    if args.params:
        if args.flavors:
            sys.exit("--params and --flavor are mutually exclusive (--params IS a "
                     "single --flavor primary=FILE)")
        if not os.path.isfile(args.params):
            sys.exit(f"--params not found: {args.params}")
        args.flavors = [("primary", args.params)]
        if args.min_relative_intensity is None:
            v = get_param(args.params, "carafe_mask_min_relative_intensity")
            if v is not None:
                args.min_relative_intensity = float(v)
        if args.min_kept_peaks is None:
            v = get_param(args.params, "carafe_mask_min_peaks")
            if v is not None:
                args.min_kept_peaks = int(v)
    if not args.flavors:
        sys.exit("At least one --flavor (or --params) required.")
    if args.min_relative_intensity is None:
        args.min_relative_intensity = 0.10
    if args.min_kept_peaks is None:
        args.min_kept_peaks = 6


def parse_flavor(value):
    name, sep, file = value.partition("=")
    if not sep or not name or not file:
        raise argparse.ArgumentTypeError(f"--flavor must be NAME=PARAMS_FILE, got "
                                         f"'{value}'")
    return name, file


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Carafe ahead-of-time pipeline driver (resumable).",
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    ap.add_argument("--fasta", required=True)
    ap.add_argument("--out", dest="out_dir", required=True)
    ap.add_argument("--comet", required=True)
    ap.add_argument("--flavor", dest="flavors", type=parse_flavor, action="append",
                    metavar="NAME=PARAMS_FILE")
    ap.add_argument("--params", default="",
                    help="single-flavor shorthand: equivalent to --flavor "
                         "primary=FILE, plus the mask thresholds are read from the "
                         "file's carafe_mask_min_relative_intensity / "
                         "carafe_mask_min_peaks keys when present (an explicit "
                         "--min-relative-intensity/--min-kept-peaks flag still wins)")
    ap.add_argument("--charges", default="")
    ap.add_argument("--include-decoys", action="store_true")
    ap.add_argument("--carafe-mode", default="phosphorylation")
    ap.add_argument("--parquet", action="store_true")
    ap.add_argument("--chunk-size", type=int, default=50000)
    ap.add_argument("--quant", choices=("u8", "u16"), default="u16")
    ap.add_argument("--min-relative-intensity", type=float, default=None)
    ap.add_argument("--min-kept-peaks", type=int, default=None)
    ap.add_argument("--workers", type=int, default=common.default_workers())
    ap.add_argument("--venv-python", default="")
    ap.add_argument("--ai-pred-py", default="")
    ap.add_argument("--stop-after", default="",
                    choices=("", "idx", "export", "convert", "predict", "cps", "mask"))
    ap.add_argument("--delete-raw", action="store_true")
    args = ap.parse_args(argv)

    apply_params_shorthand(args)

    if not os.path.isfile(args.fasta):
        sys.exit(f"--fasta not found: {args.fasta}")
    if not common.is_runnable(args.comet):
        sys.exit(f"--comet not found/executable: {args.comet}")
    for _, pf in args.flavors:
        if not os.path.isfile(pf):
            sys.exit(f"flavor params not found: {pf}")

    os.makedirs(args.out_dir, exist_ok=True)
    PrerunDriver(args).run()


if __name__ == "__main__":
    main()
