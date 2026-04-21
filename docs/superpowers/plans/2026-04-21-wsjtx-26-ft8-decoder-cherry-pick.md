# WSJT-X 2.6 FT8 Decoder Cherry-Pick into WKjTX 1.0 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring WSJT-X 2.3-2.6 FT8 decoder enhancements into WKjTX (currently on JTDX 2.2.159 base) without regressing JTDX's own enhancements (iterative passes, false-decode filter, subtraction-decode, AGC tuning), measured by aggregate decode yield on WSJT-X bundled FT8 sample WAVs.

**Architecture:** Three-way merge: WSJT-X 2.2 = common ancestor; JTDX 2.2.159 = our base (WKjTX 1.0 ships this); WSJT-X 2.6.0 = target. We extract the 2.2→2.6 patch on every `lib/ft8*` file, hand-merge each chunk into JTDX's heavily-modified equivalents one-by-one, rebuild + benchmark per chunk, keep only chunks that hold or increase yield. A reproducible Python benchmark harness wraps `wkjtx.exe` to give an objective accept/reject signal per cherry-pick.

**Tech Stack:** MSYS2 MINGW64 + ninja + Qt5 + GNU Fortran (existing WKjTX build); Python 3.11 (test harness, available at `/c/Python314/`); WSJT-X upstream git mirror at `/tmp/wsjtx-upstream`; sample WAVs from WSJT-X upstream `samples/` directory.

---

## File Structure

This plan creates a benchmark harness, then iteratively modifies the JTDX FT8 decoder. New artifacts:

- `tools/decode-bench/` — new directory for the test harness
  - `tools/decode-bench/run_bench.py` — runs `wkjtx.exe` headless against a WAV directory, parses decode lines, emits JSON
  - `tools/decode-bench/compare.py` — A/B compare two JSON results, report delta yield + new/lost decodes
  - `tools/decode-bench/fetch_samples.py` — one-time fetch of WSJT-X bundled FT8 sample WAVs into `tools/decode-bench/samples/ft8/`
  - `tools/decode-bench/README.md` — how to run
  - `tools/decode-bench/baselines/` — committed JSON snapshots of known-good yields per WKjTX commit
  - `tools/decode-bench/.gitignore` — ignore `samples/` (large, fetched on demand)

Modified files (JTDX FT8 decoder, by phase):
- `jtdx-source/lib/ft8_decode.f90` — main FT8 decode entry called by `wkjtxjt9.exe`
- `jtdx-source/lib/ft8b.f90` — JTDX's enhanced FT8 decode pass
- `jtdx-source/lib/ft8s.f90`, `ft8sd.f90`, `ft8sd1.f90`, `ft8apset.f90` — JTDX iterative passes
- `jtdx-source/lib/agccft8.f90` — JTDX FT8 AGC
- `jtdx-source/lib/chkfalse8.f90` — JTDX false-decode filter
- `jtdx-source/lib/decoder.f90` — top-level decoder dispatch
- `jtdx-source/lib/ft8v2/bpdecode174_91.f90` — LDPC decoder
- `jtdx-source/lib/ft8v2/osd174_91.f90` — Ordered Statistics Decoder
- `jtdx-source/lib/ft8v2/subtractft8.f90` — JTDX subtraction-decode

Additional WKjTX-level updates at end:
- `jtdx-source/Versions.cmake` — bump patch version (1.0.1)
- `jtdx-source/about.cpp` — add credit line for WSJT-X 2.6 decoder backports
- `CHANGELOG.md` — document the cherry-pick scope and yield results
- `README.md` — link to benchmark report

Each iteration (Phase 4) modifies one or two files at a time, rebuilds, benchmarks, commits or reverts.

---

## Working assumptions

1. The user will not commit confidential WAVs to a public repo. We use only WSJT-X upstream's MIT-compatible bundled samples in `samples/`.
2. Each rebuild takes ~20-60 seconds incrementally (Fortran files). Full rebuild ~10-30 min — to be avoided.
3. Each benchmark run takes ~30-60 seconds per WAV file. Total for the bundled sample set: ~2-5 minutes.
4. We accept a +/-2% yield variance as noise (decoder has stochastic elements when AP / averaging is enabled). Anything within the noise band is treated as "no change".
5. The user accepts that this is multi-session work spread over 2-3 weeks. Plan does NOT assume single-shot execution.
6. Branch name: all cherry-pick work happens on `decoder/wsjtx-2.6-cherry-pick` branched off `main` at v1.0.0. Final merge back to `main` produces v1.0.1 (or v1.1.0 if yield improves >5%).

---

## Phase 0: Setup branch + deeper upstream clone

### Task 0.1: Branch off v1.0.0

**Files:** none — git only

- [ ] **Step 1: Create + checkout work branch**

```bash
cd "G:/Claude Local/WKjTX"
git checkout -b decoder/wsjtx-2.6-cherry-pick v1.0.0
git log --oneline -3
```

Expected: HEAD shows the `release v1.0.0` commit on the new branch.

- [ ] **Step 2: Confirm clean tree**

```bash
git status
```

Expected: `nothing to commit, working tree clean`. If anything is dirty, stop and resolve before continuing.

### Task 0.2: Full WSJT-X upstream clone

**Files:** `/tmp/wsjtx-upstream` (clone — outside repo, do not commit)

- [ ] **Step 1: Replace shallow clone with full clone**

```bash
rm -rf /tmp/wsjtx-upstream
cd /tmp && git clone https://git.code.sf.net/p/wsjt/wsjtx wsjtx-upstream
```

Expected: clone completes, `~150MB`, all tags present.

- [ ] **Step 2: Verify tags reachable**

```bash
cd /tmp/wsjtx-upstream && git tag -l 'wsjtx-2.*' | sort -V | tail -10
```

Expected: list contains `wsjtx-2.2.0`, `wsjtx-2.2.2`, `wsjtx-2.3.0`, `wsjtx-2.3.1`, `wsjtx-2.4.0`, `wsjtx-2.5.0`, `wsjtx-2.5.4`, `wsjtx-2.6.0`, `wsjtx-2.6.1`, `wsjtx-2.7.0`.

- [ ] **Step 3: Confirm 2.2 → 2.6.0 patch on lib/ft8 is non-empty**

```bash
cd /tmp/wsjtx-upstream
git diff wsjtx-2.2.0..wsjtx-2.6.0 -- 'lib/ft8/' 'lib/ft8b.f90' 'lib/ft8_decode.f90' | wc -l
```

Expected: > 100 lines. If zero, file paths are wrong — investigate before proceeding.

---

## Phase 1: Identify candidate FT8 commits in WSJT-X 2.3 → 2.6

### Task 1.1: List all commits touching FT8 decoder paths

**Files:** none — git inspection

- [ ] **Step 1: Enumerate FT8-touching commits 2.2 → 2.6**

```bash
cd /tmp/wsjtx-upstream
git log wsjtx-2.2.0..wsjtx-2.6.0 --oneline --no-merges \
  -- 'lib/ft8/' 'lib/ft8b.f90' 'lib/ft8_decode.f90' \
     'lib/ft8sim.f90' 'lib/decoder.f90' \
  > /tmp/ft8-commits.txt
wc -l /tmp/ft8-commits.txt
```

Expected: 30-100 commits.

- [ ] **Step 2: For each commit, capture summary + insertions/deletions**

```bash
cd /tmp/wsjtx-upstream
git log wsjtx-2.2.0..wsjtx-2.6.0 --no-merges --shortstat --pretty=format:'%H|%s' \
  -- 'lib/ft8/' 'lib/ft8b.f90' 'lib/ft8_decode.f90' 'lib/decoder.f90' \
  > /tmp/ft8-commits-detail.txt
head -40 /tmp/ft8-commits-detail.txt
```

Expected: each commit followed by line `N files changed, X insertions(+), Y deletions(-)`. Use this to spot the algorithmically interesting commits (large diffs, descriptive subjects).

- [ ] **Step 3: Save the candidate list into the repo**

```bash
mkdir -p "G:/Claude Local/WKjTX/docs/superpowers/cherry-pick-2.6"
cp /tmp/ft8-commits.txt "G:/Claude Local/WKjTX/docs/superpowers/cherry-pick-2.6/all-commits.txt"
cp /tmp/ft8-commits-detail.txt "G:/Claude Local/WKjTX/docs/superpowers/cherry-pick-2.6/all-commits-detail.txt"
```

### Task 1.2: Hand-curate the candidate queue

**Files:**
- Create: `docs/superpowers/cherry-pick-2.6/queue.md`

- [ ] **Step 1: Write the curation pass**

Read `docs/superpowers/cherry-pick-2.6/all-commits-detail.txt` end to end. For each commit, classify into one of:
- **A: Decoder algorithm** — likely yield-affecting (e.g., new candidate detection, modified iteration order, tuning constants for SNR thresholds, AP enable rules)
- **B: Bug fix** — corrects a wrong path that may also affect yield
- **C: Cosmetic / config / build** — string changes, defaults, qa values not on hot path
- **D: New mode infrastructure** — Q65/FST4 — irrelevant to FT8

Write `docs/superpowers/cherry-pick-2.6/queue.md` with:
- Header explaining rationale
- Numbered list of A and B commits in chronological order, each with: SHA, date, subject, classification, file(s) touched, hypothesised yield impact (++ / + / 0 / unknown), notes on JTDX equivalent that already exists

This step is judgement-heavy. Not a placeholder — output is the actual curated list.

- [ ] **Step 2: Commit the queue**

```bash
cd "G:/Claude Local/WKjTX"
git add docs/superpowers/cherry-pick-2.6/
git commit -m "decoder: enumerate WSJT-X 2.2→2.6 FT8 commits + curated queue"
```

---

## Phase 2: Build the benchmark harness

### Task 2.1: Skeleton run_bench.py

**Files:**
- Create: `tools/decode-bench/run_bench.py`
- Create: `tools/decode-bench/.gitignore`
- Create: `tools/decode-bench/README.md`

- [ ] **Step 1: Create the directory + .gitignore**

```bash
mkdir -p "G:/Claude Local/WKjTX/tools/decode-bench/samples/ft8"
mkdir -p "G:/Claude Local/WKjTX/tools/decode-bench/baselines"
```

Write `tools/decode-bench/.gitignore`:

```
samples/
__pycache__/
*.pyc
last_run.json
```

- [ ] **Step 2: Write run_bench.py**

Write `tools/decode-bench/run_bench.py`:

```python
"""Run WKjTX decode against a WAV directory and emit a JSON report.

Usage:
  python run_bench.py --exe <path/to/wkjtx.exe> --samples <dir> --out result.json

Strategy:
- WKjTX (and JTDX) decode WAV files via the File menu, but for headless
  benchmarking we invoke the standalone `wkjtxjt9.exe` decoder subprocess
  directly with the WAV as input. wkjtxjt9 reads the wav, writes ALL.TXT-
  style decode lines to stdout, exits.
- For each WAV: capture stdout, regex-parse FT8 decode lines, dedup on
  (callsign1, callsign2, message_text), count.
- Emit JSON: {sha: <git HEAD>, exe: <path>, samples: [...], totals: {...}}
"""
import argparse, json, re, subprocess, sys
from pathlib import Path

# JTDX/WKjTX FT8 decode line format (from ALL.TXT):
#   "HHMMSS  -SS  DT  FREQ ~  MESSAGE"
# Example:
#   "163030  -8 -0.4 1234 ~  CQ DX K1JT FN20"
DECODE_RE = re.compile(
    r"^\s*(\d{6})\s+(-?\d+)\s+(-?\d+\.\d)\s+(\d+)\s+~\s+(.+?)\s*$"
)

def run_one(exe: Path, wav: Path, mode: str = "FT8") -> list[dict]:
    """Invoke wkjtxjt9 on a single WAV. Returns list of decode dicts."""
    proc = subprocess.run(
        [str(exe), "-f", str(wav), "-m", mode],
        capture_output=True, text=True, timeout=60,
    )
    decodes = []
    for line in (proc.stdout + "\n" + proc.stderr).splitlines():
        m = DECODE_RE.match(line)
        if m:
            decodes.append({
                "time": m.group(1),
                "snr": int(m.group(2)),
                "dt": float(m.group(3)),
                "freq": int(m.group(4)),
                "msg": m.group(5),
            })
    return decodes

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True, help="Path to wkjtxjt9.exe (the decoder subprocess)")
    ap.add_argument("--samples", required=True, help="Directory of FT8 WAV files")
    ap.add_argument("--out", required=True, help="Output JSON path")
    args = ap.parse_args()

    exe = Path(args.exe)
    samples_dir = Path(args.samples)
    wavs = sorted(samples_dir.glob("*.wav"))
    if not wavs:
        print(f"No WAV files in {samples_dir}", file=sys.stderr)
        return 2

    result = {"exe": str(exe), "samples_dir": str(samples_dir), "files": []}
    for wav in wavs:
        try:
            decodes = run_one(exe, wav)
        except subprocess.TimeoutExpired:
            decodes = []
            print(f"TIMEOUT {wav.name}", file=sys.stderr)
        result["files"].append({"name": wav.name, "decode_count": len(decodes), "decodes": decodes})
        print(f"{wav.name}: {len(decodes)} decodes", file=sys.stderr)

    total = sum(f["decode_count"] for f in result["files"])
    result["totals"] = {"files": len(result["files"]), "total_decodes": total}

    Path(args.out).write_text(json.dumps(result, indent=2))
    print(f"\nTotal: {total} decodes across {len(result['files'])} files. Saved {args.out}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 3: Smoke-test the script can import**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python run_bench.py --help
```

Expected: argparse usage printed. No traceback.

- [ ] **Step 4: Commit**

```bash
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/
git commit -m "decoder: scaffold benchmark harness (run_bench.py)"
```

### Task 2.2: Verify wkjtxjt9 CLI accepts the WAV input

**Files:** none — exploration

- [ ] **Step 1: Find wkjtxjt9 CLI options**

```bash
cd "G:/Claude Local/WKjTX/jtdx-source/build-wkjtx"
./wkjtxjt9.exe --help 2>&1 | head -40
```

Expected: usage with `-f <wavfile>` and `-m <mode>` options listed.

- [ ] **Step 2: Sanity-check on a known JT9 WAV (no FT8 sample yet)**

```bash
./wkjtxjt9.exe -f "../samples/JT9/130418_1742.wav" -m JT9 2>&1 | head -10
```

Expected: at least one decode line printed in JTDX format. If no decodes, the WAV may be empty or the CLI args wrong — adjust `run_bench.py` accordingly. Document the actual format observed.

- [ ] **Step 3: If CLI args differ from the assumed `-f / -m`, update run_bench.py**

If observed CLI is e.g. `wkjtxjt9 <wavfile>` positional or has different mode flag, edit `run_one()` in `run_bench.py:20` to match. Re-commit if changed:

```bash
git add tools/decode-bench/run_bench.py
git commit -m "decoder: align run_bench CLI to actual wkjtxjt9 invocation"
```

### Task 2.3: fetch_samples.py — one-time WSJT-X sample fetch

**Files:**
- Create: `tools/decode-bench/fetch_samples.py`

- [ ] **Step 1: Locate WSJT-X bundled FT8 samples**

```bash
ls /tmp/wsjtx-upstream/samples/ 2>&1
find /tmp/wsjtx-upstream/samples -name '*.wav' | head -10
```

Expected: a `FT8/` subdirectory (or files under `samples/`) with `.wav` files. If none, FT8 samples may have been moved out of the repo — fall back to fetching from `http://physics.princeton.edu/pulsar/k1jt/` or the wsjt-devel mailing list archive.

- [ ] **Step 2: Write fetch_samples.py**

Write `tools/decode-bench/fetch_samples.py`:

```python
"""Copy FT8 sample WAVs from a local WSJT-X clone into the bench samples dir.

If `--from-wsjtx` is not given, defaults to /tmp/wsjtx-upstream/samples.

Usage:
  python fetch_samples.py [--from-wsjtx /path/to/wsjtx-upstream]
"""
import argparse, shutil, sys
from pathlib import Path

DEFAULT_SOURCE = Path("/tmp/wsjtx-upstream/samples")
DEFAULT_DEST = Path(__file__).parent / "samples" / "ft8"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-wsjtx", default=str(DEFAULT_SOURCE))
    args = ap.parse_args()

    src = Path(args.from_wsjtx)
    if not src.exists():
        print(f"Source not found: {src}", file=sys.stderr)
        return 2

    DEFAULT_DEST.mkdir(parents=True, exist_ok=True)

    # Find FT8 wavs anywhere under the source (the path may be FT8/ or
    # samples/FT8/ depending on WSJT-X release).
    candidates = list(src.rglob("*.wav"))
    ft8_wavs = [w for w in candidates if "ft8" in str(w).lower()]

    if not ft8_wavs:
        # Some releases put samples without FT8 in the path. Copy
        # everything as a fallback and warn.
        ft8_wavs = candidates
        print(f"No 'ft8' in path — copying ALL {len(candidates)} sample WAVs", file=sys.stderr)

    for wav in ft8_wavs:
        target = DEFAULT_DEST / wav.name
        shutil.copy2(wav, target)
        print(f"  + {wav.name} ({wav.stat().st_size} bytes)")

    print(f"\nCopied {len(ft8_wavs)} WAVs to {DEFAULT_DEST}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 3: Run it**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python fetch_samples.py
ls samples/ft8/ 2>&1 | head -10
```

Expected: at least one `.wav` file in `samples/ft8/`. If empty, follow up: WSJT-X FT8 samples may need to be downloaded from a separate URL (check `samples/CMakeLists.txt` in `/tmp/wsjtx-upstream` for download URLs).

- [ ] **Step 4: Commit**

```bash
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/fetch_samples.py
git commit -m "decoder: fetch_samples.py one-shot WSJT-X sample copy"
```

### Task 2.4: compare.py A/B JSON diff

**Files:**
- Create: `tools/decode-bench/compare.py`

- [ ] **Step 1: Write compare.py**

Write `tools/decode-bench/compare.py`:

```python
"""Compare two run_bench JSON outputs. Reports total delta, per-file delta,
new decodes (B has, A missing), lost decodes (A has, B missing).

Usage:
  python compare.py baseline.json after.json
"""
import json, sys
from pathlib import Path

def msg_key(d: dict) -> str:
    """Dedup key. We deliberately ignore SNR/DT/freq jitter — same callsign
    pair + same message body counts as the same decode."""
    return f"{d['msg']}"

def files_to_keys(report: dict) -> dict[str, set[str]]:
    out = {}
    for f in report["files"]:
        out[f["name"]] = {msg_key(d) for d in f["decodes"]}
    return out

def main():
    if len(sys.argv) != 3:
        print("usage: compare.py baseline.json after.json", file=sys.stderr)
        return 2

    a = json.loads(Path(sys.argv[1]).read_text())
    b = json.loads(Path(sys.argv[2]).read_text())

    a_keys = files_to_keys(a)
    b_keys = files_to_keys(b)

    print(f"Baseline: {a['totals']['total_decodes']} decodes")
    print(f"After:    {b['totals']['total_decodes']} decodes")
    delta = b["totals"]["total_decodes"] - a["totals"]["total_decodes"]
    print(f"Delta:    {delta:+d}")
    print()

    all_files = sorted(set(a_keys) | set(b_keys))
    total_new = 0
    total_lost = 0
    for fname in all_files:
        ak = a_keys.get(fname, set())
        bk = b_keys.get(fname, set())
        new = bk - ak
        lost = ak - bk
        total_new += len(new)
        total_lost += len(lost)
        if new or lost:
            print(f"  {fname}: +{len(new)} new, -{len(lost)} lost")
            for n in sorted(new):
                print(f"     + {n}")
            for n in sorted(lost):
                print(f"     - {n}")

    print()
    print(f"Total new:  {total_new}")
    print(f"Total lost: {total_lost}")
    print(f"Net:        {total_new - total_lost:+d}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Smoke-test**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python compare.py
```

Expected: usage error printed (correct, no args given). No traceback.

- [ ] **Step 3: Commit**

```bash
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/compare.py
git commit -m "decoder: compare.py A/B yield diff"
```

### Task 2.5: README.md for the harness

**Files:**
- Create: `tools/decode-bench/README.md`

- [ ] **Step 1: Write the README**

Write `tools/decode-bench/README.md`:

```markdown
# WKjTX decode benchmark

Reproducible A/B test harness for FT8 decoder changes.

## One-time setup

```
python fetch_samples.py            # copies WSJT-X bundled FT8 WAVs into samples/ft8/
```

## Run a benchmark

```
python run_bench.py \
  --exe ../../jtdx-source/build-wkjtx/wkjtxjt9.exe \
  --samples samples/ft8 \
  --out result.json
```

## Compare two runs

```
python compare.py baselines/v1.0.0.json result.json
```

Reports total delta + per-file new/lost decodes.

## Convention

For each commit on `decoder/wsjtx-2.6-cherry-pick`, save the post-rebuild
result as `baselines/<short-sha>.json`. Reviewers can re-diff any pair.
```

- [ ] **Step 2: Commit**

```bash
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/README.md
git commit -m "decoder: bench harness README"
```

---

## Phase 3: Establish baseline (WKjTX 1.0.0 = JTDX 2.2.159 yield)

### Task 3.1: Run the baseline benchmark

**Files:**
- Create: `tools/decode-bench/baselines/v1.0.0.json`

- [ ] **Step 1: Confirm we're on v1.0.0 commit (no decoder changes yet)**

```bash
cd "G:/Claude Local/WKjTX"
git log --oneline -1
git rev-parse HEAD
```

Expected: HEAD points to the `release v1.0.0` commit on `decoder/wsjtx-2.6-cherry-pick` branch.

- [ ] **Step 2: Run the benchmark**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python run_bench.py \
  --exe ../../jtdx-source/build-wkjtx/wkjtxjt9.exe \
  --samples samples/ft8 \
  --out baselines/v1.0.0.json
```

Expected: per-file decode counts printed, then total. Total > 0 (else samples broken or CLI broken).

- [ ] **Step 3: Inspect totals manually**

```bash
python -c "import json; d=json.load(open('baselines/v1.0.0.json')); print(d['totals']); print(len(d['files']),'files')"
```

Expected: `{'files': N, 'total_decodes': M}` with N matching the sample file count and M plausibly 5-50 per file depending on signal density.

- [ ] **Step 4: Commit baseline**

```bash
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/baselines/v1.0.0.json
git commit -m "decoder: baseline yield from v1.0.0 (JTDX 2.2.159 decoder)"
```

### Task 3.2: Run baseline TWICE and confirm reproducibility

**Files:** none — variance check

- [ ] **Step 1: Re-run benchmark to a temp file**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python run_bench.py \
  --exe ../../jtdx-source/build-wkjtx/wkjtxjt9.exe \
  --samples samples/ft8 \
  --out /tmp/repeat.json
```

- [ ] **Step 2: Compare against committed baseline**

```bash
python compare.py baselines/v1.0.0.json /tmp/repeat.json
```

Expected: `Delta: 0` and `Total new: 0`, `Total lost: 0`. If non-zero, decoder is non-deterministic for some passes — record the noise floor (e.g., +/-1 decode is acceptable noise) and document in `tools/decode-bench/README.md` so we know the accept/reject threshold for cherry-picks.

If variance > 2 decodes total, STOP. Investigate before proceeding — non-determinism > 2 decodes makes A/B unfalsifiable for small wins.

- [ ] **Step 3: If variance documented, append to README and commit**

```bash
# only if README needed updating
cd "G:/Claude Local/WKjTX"
git add tools/decode-bench/README.md
git commit -m "decoder: document benchmark variance floor"
```

---

## Phase 4: Iterative cherry-pick loop

This is the main work phase. Loop pattern: pick top candidate from `queue.md` → extract → apply → rebuild → benchmark → keep or revert. Each iteration is one task. The plan documents the FIRST iteration in full so the engineer knows the pattern; subsequent iterations follow the same template until queue is exhausted.

### Task 4.1: Cherry-pick iteration #1 (highest-priority candidate)

**Files (per iteration — exact files set by candidate commit):**
- Modify: 1-3 of `jtdx-source/lib/ft8*.f90` files
- Add: `tools/decode-bench/baselines/<short-sha>.json`

- [ ] **Step 1: Identify the top candidate**

Open `docs/superpowers/cherry-pick-2.6/queue.md`. Pick the topmost A-classified entry not yet processed. Note its SHA.

```bash
TOP_SHA=<paste sha here>
cd /tmp/wsjtx-upstream && git show --stat $TOP_SHA
```

Expected: small commit, 1-3 files in `lib/ft8`, with a clearly decoder-related description.

- [ ] **Step 2: Extract the diff in unified format**

```bash
cd /tmp/wsjtx-upstream
git format-patch -1 $TOP_SHA --stdout > "/tmp/wsjtx-cp-$(echo $TOP_SHA | cut -c1-8).patch"
ls -la /tmp/wsjtx-cp-*.patch
```

- [ ] **Step 3: Identify JTDX target files**

Look at the patch's `--- a/lib/ft8/<file>` paths. Map to JTDX:

| WSJT-X path | JTDX path |
|---|---|
| `lib/ft8/ft8_decode.f90` | `jtdx-source/lib/ft8_decode.f90` |
| `lib/ft8/ft8b.f90` | `jtdx-source/lib/ft8b.f90` |
| `lib/ft8/decode174_91.f90` | `jtdx-source/lib/ft8v2/bpdecode174_91.f90` (different name) |
| `lib/ft8/osd174_91.f90` | `jtdx-source/lib/ft8v2/osd174_91.f90` |
| `lib/ft8/genft8.f90` | `jtdx-source/lib/genft8.f90` |
| `lib/ft8/subtractft8.f90` | `jtdx-source/lib/ft8v2/subtractft8.f90` |

If a file in the patch has no obvious JTDX equivalent, search:

```bash
basename=<filename without dir>
find "G:/Claude Local/WKjTX/jtdx-source/lib" -name "$basename"
```

- [ ] **Step 4: Hand-merge the patch**

For each hunk in the patch:
- Open the JTDX target file in an editor.
- Find the corresponding code (line numbers will differ from WSJT-X — search by surrounding context).
- Apply the change MANUALLY. Do NOT use `git apply` or `patch` — JTDX's modifications mean line numbers and surrounding code differ.
- If a hunk affects code JTDX has rewritten (e.g., a JTDX iterative pass), examine carefully:
  - If the WSJT-X change adds new logic → port the addition.
  - If the WSJT-X change replaces logic JTDX already improved → SKIP the hunk and document why in the commit message.
  - If unclear → SKIP and flag for follow-up rather than risk regression.

- [ ] **Step 5: Rebuild**

```bash
cd "G:/Claude Local/WKjTX/jtdx-source/build-wkjtx"
export PATH="/c/msys64/mingw64/bin:$PATH"
ninja 2>&1 | tail -5
```

Expected: `[N/N] Linking CXX executable wkjtxjt9.exe` and `wkjtx.exe`. If Fortran error → fix syntax / dimension mismatch (most common: wrong array bounds when JTDX has different declarations). Fix before continuing.

- [ ] **Step 6: Benchmark**

```bash
cd "G:/Claude Local/WKjTX/tools/decode-bench"
python run_bench.py \
  --exe ../../jtdx-source/build-wkjtx/wkjtxjt9.exe \
  --samples samples/ft8 \
  --out /tmp/iter1.json
python compare.py baselines/v1.0.0.json /tmp/iter1.json
```

- [ ] **Step 7: Decide keep or revert**

Decision matrix:
- **Net delta ≥ +2 decodes, total lost ≤ 1**: KEEP — proceed to step 8.
- **Net delta in [-1, +1] (noise floor)**: KEEP if no regressions in any single file > 1; otherwise REVERT.
- **Net delta < -1 OR any single file lost > 2 decodes**: REVERT.

To revert:

```bash
cd "G:/Claude Local/WKjTX"
git checkout -- jtdx-source/lib/
# rebuild to confirm clean state
cd jtdx-source/build-wkjtx && ninja 2>&1 | tail -3
```

Mark the candidate as "rejected — caused regression" in `queue.md` and proceed to next candidate (back to step 1 with new SHA).

- [ ] **Step 8: Commit (if KEEP)**

```bash
cd "G:/Claude Local/WKjTX"
SHORT_SHA=$(echo $TOP_SHA | cut -c1-8)
cp /tmp/iter1.json tools/decode-bench/baselines/cp-$SHORT_SHA.json
git add jtdx-source/lib/ tools/decode-bench/baselines/
git commit -m "decoder(ft8): cherry-pick wsjtx@$SHORT_SHA — <commit subject>

Source: wsjtx@$SHORT_SHA — <full subject>

Yield delta vs v1.0.0 baseline:
  Total: <baseline_total> -> <new_total> (<+N>)
  New decodes:  <total_new>
  Lost decodes: <total_lost>
  Files affected: <list>

Hand-merged because JTDX's <file> diverges from WSJT-X 2.2 (added <X>).
Skipped <N> hunks: <reason>."
```

- [ ] **Step 9: Update queue.md status**

Edit `docs/superpowers/cherry-pick-2.6/queue.md`: mark this candidate as ACCEPTED with the cumulative yield delta. Commit:

```bash
git add docs/superpowers/cherry-pick-2.6/queue.md
git commit -m "decoder: queue.md — mark wsjtx@$SHORT_SHA as accepted"
```

### Task 4.2..4.N: Iterations 2..N

Repeat the Task 4.1 template for each remaining candidate from `queue.md`. Each iteration is its own task. Stop when:
- Queue is exhausted, OR
- Three consecutive candidates all reject (suggests remaining queue items target code JTDX has already optimised — diminishing returns), OR
- The user calls a wrap.

After each accepted iteration, the cumulative baseline (most recent accepted commit's `cp-XXX.json`) becomes the new comparison reference for the next iteration to avoid double-counting wins.

---

## Phase 5: Final integration & release

### Task 5.1: Cumulative benchmark report

**Files:**
- Create: `docs/superpowers/cherry-pick-2.6/RESULTS.md`

- [ ] **Step 1: Run final benchmark on the latest commit of the branch**

```bash
cd "G:/Claude Local/WKjTX/jtdx-source/build-wkjtx"
ninja  # ensure latest
cd ../../tools/decode-bench
python run_bench.py \
  --exe ../../jtdx-source/build-wkjtx/wkjtxjt9.exe \
  --samples samples/ft8 \
  --out /tmp/final.json
python compare.py baselines/v1.0.0.json /tmp/final.json | tee /tmp/final-compare.txt
```

- [ ] **Step 2: Write RESULTS.md**

Write `docs/superpowers/cherry-pick-2.6/RESULTS.md` with:
- Header: branch name, date, sample corpus summary
- Per-iteration table: SHA, subject, accept/reject, delta yield
- Cumulative final delta vs v1.0.0
- Honest assessment: "X commits accepted, Y rejected. Net yield change: +N decodes (Z%) on the WSJT-X bundled FT8 sample set. Caveat: small corpus, may not generalise to user's HF crowded-band conditions — recommend on-air A/B before declaring victory."

- [ ] **Step 3: Commit**

```bash
cd "G:/Claude Local/WKjTX"
cp /tmp/final.json tools/decode-bench/baselines/v1.0.1-final.json
git add docs/superpowers/cherry-pick-2.6/RESULTS.md tools/decode-bench/baselines/v1.0.1-final.json
git commit -m "decoder: cherry-pick final results report"
```

### Task 5.2: Bump version + about credit

**Files:**
- Modify: `jtdx-source/Versions.cmake`
- Modify: `jtdx-source/about.cpp`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Bump version**

If cumulative net delta is ≥ +5%: bump MINOR (1.0 → 1.1). Else: bump PATCH (1.0.0 → 1.0.1).

Edit `jtdx-source/Versions.cmake`. Change `WSJTX_VERSION_SUB` from `0` to `1` (or bump MINOR per above).

- [ ] **Step 2: Update about.cpp credits**

Add in `jtdx-source/about.cpp` after the existing JTDX credit block:

```cpp
"<b>FT8 decoder backports</b> (WKjTX 1.0.1+): selected commits<br>"
"from WSJT-X 2.3-2.6 hand-merged on top of JTDX's enhanced<br>"
"decoder. Yield improvement on WSJT-X bundled samples: <N>%.<br><br>"
```

(Substitute `<N>` with the actual percentage from RESULTS.md.)

- [ ] **Step 3: Update CHANGELOG.md**

Append a new section at the top of `CHANGELOG.md`:

```markdown
## [1.0.1] - YYYY-MM-DD

### Decoder improvements

Backported selected FT8 decoder commits from WSJT-X 2.3-2.6 (`<commit list>`)
into JTDX's enhanced FT8 decoder paths. Hand-merged because JTDX's iterative-
pass architecture differs from WSJT-X stock; only commits that held or
increased yield on the WSJT-X bundled FT8 sample set were kept.

Cumulative yield delta vs v1.0.0 on benchmark corpus: <+N> decodes (<+Z%>).
See `docs/superpowers/cherry-pick-2.6/RESULTS.md` for per-commit detail.

JTDX-specific enhancements (iterative passes, false-decode filter,
subtraction-decode, AGC tuning) preserved.
```

- [ ] **Step 4: Rebuild + smoke test**

```bash
cd "G:/Claude Local/WKjTX/jtdx-source/build-wkjtx"
export PATH="/c/msys64/mingw64/bin:$PATH"
cmake . 2>&1 | tail -2
ninja 2>&1 | tail -5
./wkjtx.exe --help 2>&1 | head -3
```

Expected: build succeeds, exe runs (exit 0).

- [ ] **Step 5: Commit**

```bash
cd "G:/Claude Local/WKjTX"
git add jtdx-source/Versions.cmake jtdx-source/about.cpp CHANGELOG.md
git commit -m "release: bump to v1.0.1 — FT8 decoder cherry-pick from WSJT-X 2.6"
```

### Task 5.3: Repackage portable ZIP

**Files:**
- Create: `G:/Claude Local/WKjTX-1.0.1-portable-win64.zip` (outside repo, release artefact)

- [ ] **Step 1: Reinstall to staging**

```bash
rm -rf "G:/Claude Local/WKjTX-1.0.1-portable" 2>&1
cd "G:/Claude Local/WKjTX/jtdx-source/build-wkjtx"
export PATH="/c/msys64/mingw64/bin:$PATH"
cmake --install . --prefix "G:/Claude Local/WKjTX-1.0.1-portable" 2>&1 | tail -5
```

- [ ] **Step 2: Restage translations + LEGGIMI + README**

Same content as v1.0.0 — copy from previous staging or regenerate. The README/LEGGIMI both need a one-line bump at the top mentioning v1.0.1 + the decoder backport.

- [ ] **Step 3: ZIP**

```bash
rm -f "G:/Claude Local/WKjTX-1.0.1-portable-win64.zip"
powershell.exe -NoProfile -Command "Compress-Archive -Path 'G:\Claude Local\WKjTX-1.0.1-portable' -DestinationPath 'G:\Claude Local\WKjTX-1.0.1-portable-win64.zip' -CompressionLevel Optimal -Force"
ls -lh "G:/Claude Local/WKjTX-1.0.1-portable-win64.zip"
```

### Task 5.4: Merge branch + tag + GitHub release

**Files:** none — git + gh

- [ ] **Step 1: Merge to main**

```bash
cd "G:/Claude Local/WKjTX"
git checkout main
git merge --no-ff decoder/wsjtx-2.6-cherry-pick -m "merge: FT8 decoder cherry-pick from WSJT-X 2.3-2.6"
git push origin main
```

- [ ] **Step 2: Tag**

```bash
git tag -a v1.0.1 -m "WKjTX 1.0.1 — FT8 decoder backports from WSJT-X 2.6"
git push origin v1.0.1
```

- [ ] **Step 3: GitHub release with ZIP**

```bash
gh release create v1.0.1 \
  --title "WKjTX 1.0.1 — FT8 decoder cherry-pick from WSJT-X 2.6" \
  --notes-file CHANGELOG.md \
  "G:/Claude Local/WKjTX-1.0.1-portable-win64.zip#WKjTX-1.0.1-portable-win64.zip (Windows 64-bit)"
```

Expected: release URL printed. Verify in browser that ZIP is attached.

---

## Failure modes to watch for

- **Decoder produces NEW false positives that pass the chkfalse8.f90 filter**: a cherry-pick may slip through the filter because it produces a different fingerprint. Always inspect the `+ <new decode>` lines in `compare.py` output for plausibility (real callsigns, consistent grids, sensible reports). If many "decodes" are gibberish callsigns, REVERT regardless of count.
- **Build fails with Fortran dimension mismatch**: WSJT-X may have changed an array size that JTDX kept at the old value. Either resize JTDX's array (carefully, may break JTDX passes) or skip the cherry-pick.
- **Yield improves on benchmark but degrades on user's on-air audio**: bundled samples are not crowded-band representative. After release, ask user to record 10-20 min of HF FT8 and re-benchmark privately. If yield drops, retract v1.0.1.
- **Same commit cherry-picked twice into different files** (because WSJT-X 2.6's lib/ft8/ subdir was created by reorganising lib/, and the same logic now appears in two places): use `git diff --stat` aggressively and de-dup.

## When to call the user

- Before starting: confirm the queue.md curation by sharing the top 5 candidates and rationale (1 message).
- After every 3 accepted cherry-picks: progress report with cumulative delta (1 message).
- Before final tag: share RESULTS.md draft (1 message).
- If three consecutive iterations reject: ask whether to continue or wrap (1 message).
