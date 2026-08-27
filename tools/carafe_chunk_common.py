#!/usr/bin/env python3
"""
Shared helpers for the Carafe ahead-of-time pipeline drivers (carafe_prerun.py,
run_carafe_chunked.py, build_carafe_mask_chunked.py -- all reachable through the
tools/carafe.py umbrella CLI).

These drivers were originally bash+awk (tools/*.sh, since deleted) and therefore
WSL/Linux-only; they were ported to stdlib-only Python so the whole pipeline runs
identically under WSL Ubuntu and a native Windows terminal (cmd.exe / PowerShell).
Everything in this module is deliberately dependency-free (no psutil, no pandas) --
platform-specific bits (CPU count, RSS/swap sampling, venv python layout) degrade
gracefully instead of failing where an OS facility is missing.

The two splitters preserve the exact on-disk layout of their bash/awk predecessors
(chunk_%05d.tsv naming, header prepended to every chunk, .split_done markers written by
the callers) so a workdir half-processed by the old scripts resumes seamlessly under the
Python drivers and vice versa.
"""

import os
import re
import sys
import time
from datetime import datetime, timezone


# ---------------------------------------------------------------------------
# Small platform helpers
# ---------------------------------------------------------------------------

def utc_stamp():
    """UTC timestamp in the same %FT%TZ format the bash drivers used (`date -u +%FT%TZ`)."""
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def default_workers():
    """cpus-2, floored at 1 (the bash drivers' `$(nproc) - 2` default)."""
    return max(1, (os.cpu_count() or 4) - 2)


def default_venv_python():
    """The Carafe venv python for the current platform: ~/.carafe/.venv/bin/python on
    POSIX, ~/.carafe/.venv/Scripts/python.exe on Windows. Returns the path whether or
    not it exists -- callers validate."""
    venv = os.path.join(os.path.expanduser("~"), ".carafe", ".venv")
    if os.name == "nt":
        return os.path.join(venv, "Scripts", "python.exe")
    return os.path.join(venv, "bin", "python")


def is_runnable(path):
    """Best-effort 'can this be exec'd' check. On Windows os.access(X_OK) is not
    meaningful, so existence is the whole test there."""
    if not os.path.isfile(path):
        return False
    if os.name == "nt":
        return True
    return os.access(path, os.X_OK)


# ---------------------------------------------------------------------------
# Process memory sampling (run_carafe_chunked's mem_samples.tsv)
# ---------------------------------------------------------------------------

def sample_rss_kb(pid):
    """RSS of process `pid` in KB, or None if unknowable. psutil if available, else
    /proc on Linux, else give up (Windows without psutil)."""
    try:
        import psutil  # noqa: PLC0415 -- optional dependency, probed at call time
        return psutil.Process(pid).memory_info().rss // 1024
    except Exception:
        pass
    try:
        with open(f"/proc/{pid}/status") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    return int(line.split()[1])
    except Exception:
        pass
    return None


def sample_swap_used_kb():
    """System swap used in KB, or None if unknowable (mirrors `free -k`'s Swap used)."""
    try:
        import psutil  # noqa: PLC0415 -- optional dependency, probed at call time
        return psutil.swap_memory().used // 1024
    except Exception:
        pass
    try:
        total = free = None
        with open("/proc/meminfo") as f:
            for line in f:
                if line.startswith("SwapTotal:"):
                    total = int(line.split()[1])
                elif line.startswith("SwapFree:"):
                    free = int(line.split()[1])
        if total is not None and free is not None:
            return total - free
    except Exception:
        pass
    return None


def run_memory_sampler(proc, out_path, interval_seconds=5.0):
    """Sample `proc` (a subprocess.Popen) RSS + system swap every `interval_seconds`
    into a TSV at `out_path`, until the process exits. Same three-column format as the
    bash sampler (epoch / rss_kb / swap_used_kb); unknowable values become 0, exactly
    like the bash `${rss:-0}` fallback. Intended to run on a daemon thread."""
    with open(out_path, "w", newline="\n") as f:
        f.write("epoch\trss_kb\tswap_used_kb\n")
        while proc.poll() is None:
            rss = sample_rss_kb(proc.pid)
            swap = sample_swap_used_kb()
            f.write(f"{int(time.time())}\t{rss or 0}\t{swap or 0}\n")
            f.flush()
            # Sleep in small steps so the sampler notices process exit promptly.
            deadline = time.monotonic() + interval_seconds
            while proc.poll() is None and time.monotonic() < deadline:
                time.sleep(0.2)


# ---------------------------------------------------------------------------
# Chunk splitting
# ---------------------------------------------------------------------------

def chunk_name(index):
    return f"chunk_{index:05d}"


def split_tsv_with_header(in_path, chunk_dir, chunk_size):
    """Split a headered TSV into chunk_%05d.tsv files of `chunk_size` data rows each,
    the header line prepended to every chunk -- byte-faithful port of the bash
    `head -1` + `tail -n +2 | split -l N` + reassemble sequence (binary I/O, so no
    platform newline translation can corrupt the data). Returns the chunk count."""
    os.makedirs(chunk_dir, exist_ok=True)
    n_chunks = 0
    with open(in_path, "rb") as src:
        header = src.readline()
        if header and not header.endswith(b"\n"):
            # bash's `printf '%s\n' "$HEADER"` guaranteed a terminator; match it.
            header += b"\n"
        out = None
        rows_in_chunk = 0
        for line in src:
            if out is None or rows_in_chunk == chunk_size:
                if out is not None:
                    out.close()
                out = open(os.path.join(chunk_dir, chunk_name(n_chunks) + ".tsv"), "wb")
                out.write(header)
                n_chunks += 1
                rows_in_chunk = 0
            out.write(line)
            rows_in_chunk += 1
        if out is not None:
            out.close()
    return n_chunks


def split_variant_map(vmap_path, out_dir, chunk_size, log=sys.stderr):
    """Port of tools/split_variant_map_for_chunks.awk (now retired): split an
    idx_to_carafe.py variant-map sidecar (leading '# VarModConfig: ...' comment line +
    header line + data rows whose column 1 is a GLOBAL 0-based row_index) into per-chunk
    files at the same row-count boundaries split_tsv_with_header() uses for the
    corresponding out_tsv, rewriting each row's row_index to be chunk-local (subtract
    chunk_index * chunk_size).

    Relies on the input being strictly row_index-ordered (true for idx_to_carafe.py's
    own output) so only one output file is open at a time -- single streaming pass. A
    chunk with no variant rows produces no file, exactly like the awk version. Returns
    (n_data_rows, highest_chunk_index + 1); (0, 0) for an empty map."""
    os.makedirs(out_dir, exist_ok=True)
    n_rows = 0
    cur_chunk = -1
    out = None
    with open(vmap_path, "r", newline="") as src:
        comment_line = src.readline().rstrip("\r\n")
        header_line = src.readline().rstrip("\r\n")
        for raw in src:
            line = raw.rstrip("\r\n")
            if not line:
                continue
            fields = line.split("\t")
            row_index = int(fields[0])
            chunk = row_index // chunk_size
            if chunk != cur_chunk:
                if out is not None:
                    out.close()
                cur_chunk = chunk
                out = open(os.path.join(out_dir, chunk_name(chunk) + ".tsv"),
                           "w", newline="\n")
                out.write(comment_line + "\n")
                out.write(header_line + "\n")
            fields[0] = str(row_index - chunk * chunk_size)
            out.write("\t".join(fields) + "\n")
            n_rows += 1
    if out is not None:
        out.close()
    if log is not None:
        print(f"split_variant_map: {n_rows} data rows across {cur_chunk + 1} chunk(s) "
              f"(0..{cur_chunk})", file=log)
    return n_rows, cur_chunk + 1


def list_chunk_tsvs(chunk_dir):
    """Sorted chunk_*.tsv paths in `chunk_dir` (excluding *.input.parquet etc.)."""
    if not os.path.isdir(chunk_dir):
        return []
    pat = re.compile(r"^chunk_\d+\.tsv$")
    return [os.path.join(chunk_dir, n) for n in sorted(os.listdir(chunk_dir))
            if pat.match(n)]


def count_data_rows(tsv_path):
    """Number of data rows (total lines - 1 header) in a chunk TSV, counted in binary
    so it's cheap and newline-translation-proof."""
    n = 0
    with open(tsv_path, "rb") as f:
        for _ in f:
            n += 1
    return max(0, n - 1)
