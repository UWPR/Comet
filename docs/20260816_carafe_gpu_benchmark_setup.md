# Carafe GPU benchmark: setup and run instructions

**Audience:** Claude Code running on the Windows 11 desktop with the NVIDIA GTX 1660
Super. This document is a self-contained instruction packet — follow it directly.

**Status (2026-08-22, per `docs/20260805_carafe.md` Section 6.21):** both the CPU and GPU
full runs this packet drives completed; results are in Section 6.21. The GPU-vs-CPU
prediction-diff comparison this was built for was subsequently **retired** — future
Carafe predictions are being regenerated with a more limited search space instead (see
`docs/20260805_carafe.md`'s top status note and Section 6.23-6.24), so this exact
124,863,304-row/`phospho_charge2_withNL` job is not expected to be re-run. This packet's
setup steps and `run_carafe_chunked.sh` invocations remain accurate as reference (verified
against the current script 2026-08-25), but the ~395GB output tree this produced on the
GPU machine is superseded and, per that status note, still pending deletion there.

## 1. What this is

We're comparing Carafe's `ai_pred.py` MS2-prediction throughput on GPU vs. CPU, on
the exact same real dataset, using a chunked/resumable runner
(`tools/run_carafe_chunked.sh`) that was built and validated on a CPU-only dev
machine. That machine is currently partway through a real ~124.8M-row,
phosphorylation-mode production run with this exact tool — do not wait for it to
finish; this is an independent benchmark run on different hardware.

**CPU baseline already measured** (single-threaded, forced by `ai_pred.py`'s own
`--device cpu` code path via `threadpool_limits(limits=1, ...)`), for comparison
once your GPU numbers are in:

| Condition | Rate |
|---|---|
| Solo/uncontended (first 50,000-row chunk, cold start) | 1,851.9 rows/sec |
| Sustained production average (409 chunks / 19,048s, mid-run) | ~1,074 rows/sec |

Also already measured on the CPU machine: running multiple `ai_pred.py` chunks
**concurrently made things worse**, not better (4-way parallel: ~140 rows/sec per
process, ~560 rows/sec aggregate — well under the 1,850 rows/sec solo rate).
Root cause wasn't fully isolated (suspected I/O contention on model loading, since
peak RSS stayed trivial at ~940MB/process either way — not a memory story). **Do
not assume this result transfers to GPU** — a single shared GPU has its own
resource-contention story (one device, likely time-sliced across processes unless
MPS/MIG is configured, which it almost certainly isn't here) — but don't assume
parallelism helps on GPU either. Measure it the same way we did on CPU: small
scale, both ways, before committing.

## 2. Prerequisites

**Recommended environment: WSL2**, not native Windows Python. Mirrors the already-
validated setup on the CPU dev machine exactly (same venv layout, same repo
checkout paths, same script unmodified) rather than porting anything to
PowerShell or validating `alphabase`/`peptdeep` install cleanly on native Windows.

1. **NVIDIA driver**: install the Windows-side GeForce/Studio driver with WSL
   CUDA support (this is a Windows-side install, not something done inside WSL —
   do **not** install a separate Linux NVIDIA driver inside WSL2, that will
   conflict). Verify from inside WSL2 with:
   ```bash
   nvidia-smi
   ```
   This should show the GTX 1660 Super. If it doesn't, stop here and fix the
   driver install before doing anything else.

2. **Disk space**: budget **~400GB free** on whatever drive you're working on.
   **Correction (2026-08-19, after both full runs actually completed):** the
   original ~110GB estimate below was wrong by ~3.6x — it was extrapolated from a
   single calibration chunk's output size (~34MB), but real per-chunk output size
   isn't uniform across the full run (later chunks' predicted-fragment tables
   appear to run larger, plausibly because `ai_pred.py`'s internal
   `sort_values('nAA')` groups same-length peptides together per invocation and
   longer peptides produce proportionally more fragment rows). The two real,
   independently-measured completed runs landed at **386GB (CPU)** and **395GB
   (GPU)** for the identical 2,498-chunk/124,863,304-row job — ~155MB/chunk
   average, not ~34MB/chunk. Original (wrong) breakdown kept below for the
   reasoning trail, but budget from the real total, not this math:
   ~9.5GB for the transferred input TSV, ~9.5GB again for the
   chunked-split copies of it, and ~83GB for the chunks' predicted output
   (2,498 chunks × ~34MB/chunk of `_ms2_df.tsv` + `_ms2_mz_df.tsv` +
   `_ms2_pred.tsv` + `_rt_pred.tsv`, measured directly from a real chunk's
   output on the CPU machine). Check free space before starting the full run:
   ```bash
   df -h .
   ```

3. **Clone the Comet repo, `carafe` branch:**
   ```bash
   git clone <comet-repo-url> Comet-master
   cd Comet-master
   git checkout carafe
   git log --oneline -1 tools/run_carafe_chunked.sh
   # should show commit 145b2f6a "Add tools/run_carafe_chunked.sh: chunked,
   # resumable Carafe ai_pred.py runner" (or later)
   ```
   Note: this repo's `.gitattributes` forces CRLF line endings on most tracked
   source files, but deliberately **not** on `.sh` files (a CRLF shebang line
   breaks direct execution) — `tools/run_carafe_chunked.sh` will check out as
   plain LF and should just work. If `git update-index --chmod=+x` didn't
   survive the transfer to this filesystem and `./run_carafe_chunked.sh` gives
   "Permission denied", just invoke it as `bash tools/run_carafe_chunked.sh ...`
   instead (all the instructions below already do this) — no need to fight the
   executable bit.

4. **Clone the Carafe repo** to the same relative path the script defaults to,
   or pass `--ai-pred-py` explicitly:
   ```bash
   git clone <carafe-repo-url> /mnt/c/Work/Carafe
   ```
   (If you clone it somewhere else, pass
   `--ai-pred-py /path/to/Carafe/src/main/resources/py/v2/ai_pred.py` to every
   invocation below.)

5. **Python venv with CUDA-enabled torch**, mirroring `~/.carafe/.venv` on the
   CPU machine but with a CUDA build instead of CPU-only:
   ```bash
   python3 -m venv ~/.carafe/.venv
   source ~/.carafe/.venv/bin/activate
   # match the CUDA version to your installed driver -- check nvidia-smi's
   # reported "CUDA Version" first, then pick the matching torch index, e.g.:
   pip install torch --index-url https://download.pytorch.org/whl/cu121
   pip install pandas numpy alphabase peptdeep threadpoolctl psutil
   ```
   **Verify CUDA is actually visible to torch before doing anything else:**
   ```bash
   python3 -c "import torch; print('cuda available:', torch.cuda.is_available()); print('device:', torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'NONE')"
   ```
   This must print `cuda available: True` and the GTX 1660 Super's name. If it
   doesn't, stop and fix the torch/CUDA install before running anything below —
   `--device cuda` will silently misbehave or error otherwise.

6. **Receive the transferred input file.** The user is moving
   `phospho_charge2_withNL_carafe_peptides.tsv` (9.49GB, 124,863,304 data rows +
   1 header, tab-separated columns `sequence  mods  mod_sites  charge`) to this
   machine manually. Confirm it has landed and looks right before proceeding:
   ```bash
   wc -l phospho_charge2_withNL_carafe_peptides.tsv   # should read 124863305
   head -3 phospho_charge2_withNL_carafe_peptides.tsv
   ```

## 3. Calibration run (do this before anything larger)

Same discipline as the CPU machine: one small chunk first, verify it's healthy,
*then* scale up. Do not skip to the full run.

```bash
mkdir -p carafe_chunked_gpu
bash tools/run_carafe_chunked.sh \
  --in phospho_charge2_withNL_carafe_peptides.tsv \
  --out carafe_chunked_gpu \
  --chunk-size 50000 \
  --mode phosphorylation \
  --device cuda \
  --tf-type ms2 \
  --limit-chunks 1 \
  --jobs 1
```

This will first split the whole 124.8M-row file into 50,000-row chunks (one-time
cost, several minutes — it's a full pass over a 9.49GB file), then run exactly
one chunk through `ai_pred.py --device cuda`.

**Check the result:**
```bash
cat carafe_chunked_gpu/chunk_preds/chunk_00000/.elapsed_seconds
cat carafe_chunked_gpu/chunk_preds/chunk_00000/.rate_rows_per_sec
cat carafe_chunked_gpu/chunk_preds/chunk_00000/ai_pred.log   # should be empty/clean, no errors
ls -la carafe_chunked_gpu/chunk_preds/chunk_00000/            # should have 4 non-empty .tsv files + markers
```

Compare `.rate_rows_per_sec` against the CPU baselines in §1 (1,851.9 rows/sec
solo, ~1,074 rows/sec sustained). This is the headline number to report back.

Also worth a quick look at GPU utilization/memory while a chunk runs (open a
second terminal):
```bash
watch -n 1 nvidia-smi
```
Note peak GPU memory used and whether utilization is pinned near 100% (compute-
bound, expected) or low (would suggest a bottleneck elsewhere, e.g. the same
kind of contention seen in the CPU parallel test, or slow disk I/O feeding the
model).

## 4. Small parallelism test (optional but recommended)

Before committing to a `--jobs` choice for the full run, test 2-way and maybe
4-way concurrency at small scale, exactly like the CPU machine did — and don't
assume the CPU finding ("parallel is worse") transfers, but don't assume the
opposite either:

```bash
bash tools/run_carafe_chunked.sh \
  --in phospho_charge2_withNL_carafe_peptides.tsv \
  --out carafe_chunked_gpu \
  --chunk-size 50000 --mode phosphorylation --device cuda --tf-type ms2 \
  --limit-chunks 4 --jobs 4
```

Compare each chunk's `.rate_rows_per_sec` (per-process rate) and the *aggregate*
throughput (4 × 50,000 rows ÷ wall-clock span) against the solo run in §3. If
aggregate throughput is meaningfully higher than solo, use that `--jobs` value
for the full run below; if it's flat or worse, stick with `--jobs 1`.

## 5. Full run

Once calibration looks healthy, launch the full remaining set. Use `nohup` and
background it — this will likely still run for multiple hours even accelerated,
and should survive a terminal/session disconnect:

```bash
nohup bash tools/run_carafe_chunked.sh \
  --in phospho_charge2_withNL_carafe_peptides.tsv \
  --out carafe_chunked_gpu \
  --chunk-size 50000 \
  --mode phosphorylation \
  --device cuda \
  --tf-type ms2 \
  --limit-chunks 0 \
  --jobs <whatever §4 concluded, default 1> \
  > carafe_gpu_full_run.log 2>&1 &
disown
echo "driver PID: $!"
```

**For progress/ETA tracking, don't write a separate timestamp file and `date -d`
it back later** — that exact approach broke on the CPU machine (a file that had
just been written and read back successfully vanished minutes later, a
filesystem write-durability quirk on `/mnt/c`-style mounts; if the GPU machine
runs WSL2 with a similar mount, it could hit the same thing). Instead get
elapsed time directly from the still-running process, which doesn't depend on
any file surviving:
```bash
ps -o etimes= -p <driver PID>          # elapsed seconds, straight from the kernel
ls carafe_chunked_gpu/chunk_preds/*/.done 2>/dev/null | wc -l   # chunks done
```
2,498 total chunks. `rate = chunks_done * 50000 / etimes` gives rows/sec;
`(2498 - chunks_done) / (chunks_done / etimes)` gives estimated remaining
chunks in seconds.

If you do hit a mysterious "No such file or directory" on a file you just
created/moved on this filesystem, and `ls` shows a `-?????????` ghost entry for
it, that's the same DrvFs directory-cache glitch seen on the CPU machine —
usually clears in under a minute; if not, just recreate the file rather than
fighting it.

## 6. What to report back

A short summary covering:
- Calibration: solo `.rate_rows_per_sec` for `chunk_00000` on `--device cuda`.
- Parallelism test result (§4): did concurrent chunks help, hurt, or do nothing
  on this GPU, and what `--jobs` value was used for the full run as a result.
- Full run: total wall time (or `.elapsed_seconds` summed across all chunks,
  whichever is cleaner), and the resulting rows/sec for the complete
  124,863,304-row set.
- Peak GPU memory observed (from `nvidia-smi` while running) — the GTX 1660
  Super has 6GB VRAM; worth confirming it wasn't a binding constraint.
- Any chunk that ended up in `carafe_gpu_full_run.log` marked `FAILED` (the
  driver logs and continues past single-chunk failures rather than aborting the
  whole run — check for these explicitly, a clean run won't have any).

That's the full CPU-vs-GPU comparison this exercise is for.
