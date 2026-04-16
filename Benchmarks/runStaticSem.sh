#!/bin/bash

export ROOT=$(pwd)
declare -a BENCHS=()


allBenchs=$(find . -maxdepth 1 -type d)
for bench in $allBenchs; do
    if [[ "$bench" == "." || "$bench" == "./baselines" ]]; then
        continue
    fi
    
    semResult=`find $bench -name export_symbolic_summary.json*`
    if [ -n "$semResult" ]; then
        continue
    fi

    echo ">>> Add benchmark $bench into the worklist..."
    BENCHS+=("$bench")
done

# Start initial processes
for ((i = 0; i < ${#BENCHS[@]}; i++)); do
    bench="${BENCHS[i]}"
    python -m semlearn -t -a "$bench"
done