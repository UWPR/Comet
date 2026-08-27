#!/usr/bin/env python3
"""
Splits a Carafe idx_to_carafe.py peptide TSV (sequence/mods/mod_sites/charge) into
fixed-size row chunks and runs Carafe's real ai_pred.py over each chunk as a separate
process/invocation, instead of one monolithic all-rows-in-memory call. Written after
phospho_charge2_withNL_carafe died silently after ~51h wall time with zero bytes written
anywhere (see chat 2026-08-16): ai_pred.py reads its whole --in_file into one pandas
DataFrame, does one single-threaded (--device cpu) predict_ms2() call over the whole
thing, and writes nothing to disk until that entire call returns -- so a 124.8M-row
input is an all-or-nothing multi-day black box with no progress signal and no resume
point if interrupted.

This driver gives each chunk its own process, its own output files, and a ".done"
marker -- so progress is visible, memory is bounded per-chunk instead of for the whole
124.8M-row set at once, and a killed/interrupted run resumes at the next incomplete
chunk instead of restarting from zero.

(Python port of the original tools/run_carafe_chunked.sh, so the pipeline also runs in
a native Windows terminal. Marker/output layout is identical to the bash era's, so a
workdir started under the old driver resumes under this one. Normally invoked through
the umbrella CLI: `tools/carafe.py predict ...`.)

ai_pred.py's --device cpu path wraps predict_ms2() in threadpool_limits(limits=1, ...)
for BLAS/OpenMP regardless of machine core count (confirmed by reading its source), so
multiple chunks can safely run as concurrent OS processes (--jobs N) without fighting
each other for the same thread pool -- each process is its own single-threaded worker.
In practice, though, measured concurrent throughput on this project's dev machine was
*worse* than serial (4-way parallel: ~140 rows/sec/process, ~560 rows/sec aggregate,
vs. ~1850 rows/sec running alone) -- something outside threadpool_limits's BLAS/OpenMP
scope contends across processes (suspected: model-loading I/O over a slow filesystem
bridge). Default --jobs 1; only raise it after calibrating on the target machine, and
don't assume it will help -- measure first.

Usage:
  tools/carafe.py predict --in FILE --out DIR [options]

Options (all have defaults suitable for a first calibration run):
  --in FILE            input peptide TSV (idx_to_carafe.py output). Required.
  --out DIR            output directory (created if missing). Required.
  --chunk-size N       data rows per chunk (default: 50000 -- calibration-sized;
                       raise for the real run once a rate is known)
  --mode MODE          ai_pred.py --mode (default: phosphorylation, matching
                       the dead run)
  --device DEV         ai_pred.py --device (default: cpu -- no GPU in this env)
  --tf-type TYPE       ai_pred.py --tf_type (default: ms2). NOTE: despite the
                       name, every tf_type branch in ai_pred.py's main() calls
                       both predict_rt() AND predict_ms2() regardless -- the
                       branches only differ in which model_dir each step uses.
                       There is no CLI-level way to skip RT prediction; its
                       output (*_rt_pred.tsv) is simply unused by
                       carafe_ms2_to_fi_mask.py, not absent. (Measured cost
                       of including it is small relative to MS2 prediction,
                       so this is not worth working around.)
  --parquet            transient parquet mode (docs/20260826_carafe.md): converts
                       each input chunk to parquet inline (cached as
                       chunk_NNNNN.input.parquet) and runs ai_pred.py --fast, so
                       prediction output lands as parquet (~5-9x smaller -- the
                       ~390GB full-proteome-phospho transient high-water mark
                       becomes ~45GB). carafe_pred_to_cps.py auto-detects parquet
                       chunk outputs. Requires pandas+pyarrow in the --venv-python
                       environment (the Carafe venv has them).
  --limit-chunks N     stop after N *newly run* chunks this invocation
                       (default: 1 -- calibration-safe; pass 0 for "run all
                       remaining chunks")
  --jobs N             concurrent ai_pred.py processes (default: 1 -- see
                       the parallelism note above before raising this)
  --venv-python PATH   python to invoke ai_pred.py with (default:
                       ~/.carafe/.venv/bin/python, or Scripts\\python.exe on
                       Windows)
  --ai-pred-py PATH    path to ai_pred.py (default: probes a Carafe checkout
                       next to this repo, then /mnt/c/Work/Carafe and
                       C:\\Work\\Carafe)

Each chunk's output lands in OUT/chunk_preds/chunk_NNNNN/, with:
  ai_pred.log             ai_pred.py's stdout+stderr
  mem_samples.tsv         epoch_seconds, ai_pred.py RSS (KB), system swap used (KB),
                          sampled every 5s while the process runs (best-effort: the
                          columns read 0 where the platform offers no probe --
                          Windows without psutil installed)
  .start_time / .end_time UTC timestamps
  .elapsed_seconds / .rate_rows_per_sec
  .done                   written only after ai_pred.py exits 0 -- presence of
                          this file is what makes a chunk "already done" and
                          skippable on the next invocation

Concatenating completed chunks' *_ms2_df.tsv / *_ms2_pred.tsv into single files for
carafe_ms2_to_fi_mask.py is a separate follow-up step, not done by this script -- run
this script to completion (or however many chunks you want) first.
"""

import argparse
import os
import subprocess
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import carafe_chunk_common as common  # noqa: E402

# ai_pred.py --fast reads --in_file with read_parquet() too, so the input chunk must be
# converted first (trivial: ~0.1s and ~14x smaller than the TSV). Run under the Carafe
# venv python, which has pandas+pyarrow. Same code the bash heredoc ran.
PARQUET_CONVERT_SNIPPET = """\
import sys
import pandas as pd
src, dst = sys.argv[1], sys.argv[2]
df = pd.read_csv(src, sep="\\t", low_memory=False, dtype={"mod_sites": str, "mods": str})
df["mods"] = df["mods"].fillna("")
df["mod_sites"] = df["mod_sites"].fillna("")
df.to_parquet(dst, compression="zstd")
"""


def find_default_ai_pred_py():
    here = os.path.dirname(os.path.abspath(__file__))
    rel = ("src", "main", "resources", "py", "v2", "ai_pred.py")
    candidates = [
        os.path.join(here, "..", "..", "Carafe", *rel),
        os.path.join("/mnt/c/Work/Carafe", *rel),
        os.path.join("C:\\Work\\Carafe", *rel),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return os.path.normpath(c)
    return candidates[1]  # keep the historical default in the error message


def write_text(path, text):
    with open(path, "w", newline="\n") as f:
        f.write(text + "\n")


def run_chunk(chunk, args, pred_dir):
    base = os.path.basename(chunk)[:-len(".tsv")]
    chunk_out = os.path.join(pred_dir, base)
    os.makedirs(chunk_out, exist_ok=True)

    if os.path.isfile(os.path.join(chunk_out, ".done")):
        print(f"[{base}] already done, skipping")
        return True

    nrows = common.count_data_rows(chunk)
    print(f"[{base}] starting: {nrows} rows, mode={args.mode} device={args.device} "
          f"tf_type={args.tf_type} parquet={int(args.parquet)}, {common.utc_stamp()}")
    write_text(os.path.join(chunk_out, ".start_time"), common.utc_stamp())
    start_ts = time.monotonic()

    in_file = chunk
    extra_args = []
    if args.parquet:
        pq_in = chunk[:-len(".tsv")] + ".input.parquet"
        if not os.path.isfile(pq_in):
            with open(os.path.join(chunk_out, "parquet_convert.log"), "wb") as log:
                subprocess.run([args.venv_python, "-c", PARQUET_CONVERT_SNIPPET,
                                chunk, pq_in],
                               stdout=log, stderr=subprocess.STDOUT, check=False)
            if not os.path.isfile(pq_in):
                print(f"[{base}] FAILED converting input to parquet -- see "
                      f"{os.path.join(chunk_out, 'parquet_convert.log')}")
                return False
        in_file = pq_in
        extra_args = ["--fast"]

    with open(os.path.join(chunk_out, "ai_pred.log"), "wb") as log:
        proc = subprocess.Popen(
            [args.venv_python, args.ai_pred_py,
             "--in_file", in_file,
             "--out_dir", chunk_out,
             "--out_prefix", base,
             "--mode", args.mode,
             "--device", args.device,
             "--tf_type", args.tf_type] + extra_args,
            stdout=log, stderr=subprocess.STDOUT)

        # Background memory sampler: RSS of the ai_pred.py process + system swap used,
        # every 5s -- direct evidence for/against a swap-thrashing failure mode,
        # instead of guessing after the fact.
        sampler = threading.Thread(
            target=common.run_memory_sampler,
            args=(proc, os.path.join(chunk_out, "mem_samples.tsv")),
            daemon=True)
        sampler.start()
        rc = proc.wait()
        sampler.join(timeout=10)

    elapsed = int(round(time.monotonic() - start_ts))
    write_text(os.path.join(chunk_out, ".end_time"), common.utc_stamp())
    write_text(os.path.join(chunk_out, ".elapsed_seconds"), str(elapsed))

    if rc != 0:
        print(f"[{base}] FAILED (exit {rc}) after {elapsed}s -- see "
              f"{os.path.join(chunk_out, 'ai_pred.log')}")
        return False

    peak_rss = peak_swap = 0
    try:
        with open(os.path.join(chunk_out, "mem_samples.tsv")) as f:
            next(f, None)
            for line in f:
                parts = line.rstrip("\r\n").split("\t")
                if len(parts) >= 3:
                    peak_rss = max(peak_rss, int(parts[1] or 0))
                    peak_swap = max(peak_swap, int(parts[2] or 0))
    except OSError:
        pass
    rate = f"{nrows / elapsed:.1f}" if elapsed > 0 else "n/a"
    write_text(os.path.join(chunk_out, ".rate_rows_per_sec"), rate)
    # .done is the resume marker -- write it last, only on success.
    write_text(os.path.join(chunk_out, ".done"), "")
    print(f"[{base}] done in {elapsed}s ({rate} rows/sec), peak RSS {peak_rss} KB, "
          f"peak swap-used {peak_swap} KB")
    return True


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Chunked Carafe ai_pred.py driver (resumable).",
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    ap.add_argument("--in", dest="in_tsv", required=True)
    ap.add_argument("--out", dest="out_dir", required=True)
    ap.add_argument("--chunk-size", type=int, default=50000)
    ap.add_argument("--mode", default="phosphorylation")
    ap.add_argument("--device", default="cpu")
    ap.add_argument("--tf-type", default="ms2")
    ap.add_argument("--parquet", action="store_true")
    ap.add_argument("--limit-chunks", type=int, default=1)
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--venv-python", default=common.default_venv_python())
    ap.add_argument("--ai-pred-py", default=find_default_ai_pred_py())
    args = ap.parse_args(argv)

    if not os.path.isfile(args.in_tsv):
        sys.exit(f"Input file not found: {args.in_tsv}")
    if not common.is_runnable(args.venv_python):
        sys.exit(f"venv python not found/executable: {args.venv_python}")
    if not os.path.isfile(args.ai_pred_py):
        sys.exit(f"ai_pred.py not found: {args.ai_pred_py}")

    chunk_dir = os.path.join(args.out_dir, "chunks")
    pred_dir = os.path.join(args.out_dir, "chunk_preds")
    os.makedirs(chunk_dir, exist_ok=True)
    os.makedirs(pred_dir, exist_ok=True)

    # ---- 1. Split (idempotent) ----
    split_marker = os.path.join(chunk_dir, ".split_done")
    if not os.path.isfile(split_marker):
        print(f"[split] splitting {args.in_tsv} into {args.chunk_size}-row chunks "
              f"under {chunk_dir} ...")
        n_chunks = common.split_tsv_with_header(args.in_tsv, chunk_dir, args.chunk_size)
        write_text(split_marker, "")
        print(f"[split] done: {n_chunks} chunks")
    else:
        print(f"[split] already split (found {split_marker}), reusing existing chunks")

    # ---- 2. Drive chunks, honoring --limit-chunks and --jobs ----
    all_chunks = common.list_chunk_tsvs(chunk_dir)
    todo = [c for c in all_chunks
            if not os.path.isfile(os.path.join(
                pred_dir, os.path.basename(c)[:-len(".tsv")], ".done"))]
    print(f"[driver] {len(all_chunks)} total chunks, {len(todo)} not yet done")

    if args.limit_chunks != 0 and len(todo) > args.limit_chunks:
        todo = todo[:args.limit_chunks]
    print(f"[driver] running {len(todo)} chunk(s) this invocation, jobs={args.jobs}")

    # A failed chunk must not abort the multi-hour run -- log it and keep going; it
    # stays undone (no .done marker) and will be retried on the next invocation.
    if args.jobs <= 1:
        for c in todo:
            if not run_chunk(c, args, pred_dir):
                print(f"[driver] {os.path.basename(c)[:-len('.tsv')]} FAILED, "
                      f"continuing to next chunk")
    else:
        from concurrent.futures import ThreadPoolExecutor  # noqa: PLC0415
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = {pool.submit(run_chunk, c, args, pred_dir): c for c in todo}
            for fut, c in futures.items():
                if not fut.result():
                    print(f"[driver] {os.path.basename(c)[:-len('.tsv')]} FAILED, "
                          f"continuing")

    print("[driver] invocation complete.")


if __name__ == "__main__":
    main()
