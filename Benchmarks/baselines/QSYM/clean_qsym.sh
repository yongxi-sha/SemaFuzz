#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-$(pwd)}"

# Bench dirs you listed (kept explicit on purpose)
BENCHES=(
  arrow avconv exiv json libpng libxml mdp mp3gain mp4v2 pycdc sablot tcpdump
  tippecanoe uriparser xpdf
)

echo "[+] Cleanup root: $ROOT"
echo "[+] Targets: ${BENCHES[*]}"

for b in "${BENCHES[@]}"; do
  d="$ROOT/$b"
  [[ -d "$d" ]] || { echo "[-] Skip (missing): $d"; continue; }

  echo
  echo "[*] Benchmark: $b"

  # 1) Remove files like .733653 .70 .74 etc under the benchmark directory (not directories)
  echo "    - Removing dot-number files (.[1-9]*):"
  find "$d" -maxdepth 1 -type f -name ".[1-9]*" -print -delete || true

  # If those files might also appear inside fuzz/ (often true), remove there too:
  if [[ -d "$d/fuzz" ]]; then
    echo "    - Removing dot-number files inside fuzz/:"
    find "$d/fuzz" -type f -name ".[1-9]*" -print -delete || true
  fi

  # 2) Remove fuzz/out
  if [[ -d "$d/fuzz/out" ]]; then
    echo "    - Removing fuzz/out:"
    rm -rf "$d/fuzz/out"
  else
    echo "    - fuzz/out not found (ok)"
  fi
done

echo
echo "[+] Done."

