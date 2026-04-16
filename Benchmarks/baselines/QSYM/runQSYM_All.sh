#!/bin/bash
set -euo pipefail

# ===== config =====
ROOT="$(pwd)"              # benchmarks root
PAR=5                      # run 5 fuzzing instances at a time
DUR="24h"                  # per-benchmark time limit
LOGDIR="${ROOT}/_batch_logs_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOGDIR"

# Find all runFuzz.sh files (1 per benchmark directory)
mapfile -t SCRIPTS < <(find "$ROOT" -mindepth 2 -maxdepth 2 -type f -name runFuzz.sh | sort)

if [[ ${#SCRIPTS[@]} -eq 0 ]]; then
  echo "[!] No runFuzz.sh found under $ROOT/*/runFuzz.sh"
  exit 1
fi

echo "[*] Found ${#SCRIPTS[@]} benchmarks"
echo "[*] Logs: $LOGDIR"
echo "[*] Parallelism: $PAR, Duration: $DUR each"

run_one() {
  local script="$1"
  local bdir
  bdir="$(dirname "$script")"
  local bname
  bname="$(basename "$bdir")"
  local log="${LOGDIR}/${bname}.log"

  echo "[*] START $bname  ($(date))" | tee -a "$log"
  # Run in its benchmark dir; hard-stop after 24h.
  # Use bash explicitly in case script isn't executable.
  ( cd "$bdir" && timeout -k 5m "$DUR" bash ./runFuzz.sh ) >>"$log" 2>&1
  local rc=$?
  echo "[*] END   $bname  rc=$rc  ($(date))" | tee -a "$log"
  return 0
}

export -f run_one
export LOGDIR DUR

# Run up to 4 at once
printf "%s\n" "${SCRIPTS[@]}" | xargs -n 1 -P "$PAR" -I {} bash -lc 'run_one "$@"' _ {}
echo "[*] ALL DONE ($(date))"

