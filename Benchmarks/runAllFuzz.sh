#!/bin/bash

export ROOT=$(pwd)
Model=$1 #"ds-7b"
NPROC=$2
TFLAG=$3
SEMFLAG=$4

echo "@@help: runAllFuzz.sh Model[ds-7b]->$Model NPROC[1-32]->$NPROC TFLAG[-t]->$TFLAG SEMFLAG[-l/-x/-r]->$SEMFLAG"

declare -a PIDS=()
declare -a BENCHS=()

# Collect all directories (benchmarks) to process
allBenchs=$(find . -maxdepth 1 -type d)
for bench in $allBenchs; do
    if [[ "$bench" == "." || "$bench" == "./baselines" ]]; then
        continue
    fi

    covFile=func_cov.txt
    if [ -f "$bench/$covFile" ]; then
        echo ">>>>>> $bench already has $covFile, do not add to the worklist ..."
        continue
    fi
    BENCHS+=("$bench")
done

# Start initial processes
for ((i = 0; i < NPROC && i < ${#BENCHS[@]}; i++)); do
    echo "--> runFuzz @${BENCHS[i]}"
    python -m semlearn -d $TFLAG $SEMFLAG -a "${BENCHS[i]}" "$Model" &
    PIDS+=($!)
    unset BENCHS[i]
done

# Monitor processes
while [[ ${#PIDS[@]} -gt 0 || ${#BENCHS[@]} -gt 0 ]]; do
    for ((i = 0; i < ${#PIDS[@]}; i++)); do
        if ! kill -0 "${PIDS[i]}" 2>/dev/null; then
            # Process has exited
            echo "Process ${PIDS[i]} has exited."
            unset PIDS[i]

            # Start a new process if there are more benchmarks to process
            if [[ ${#BENCHS[@]} -gt 0 ]]; then
                next_bench=${BENCHS[0]}
                echo "--> runFuzz @$next_bench"
                python -m semlearn -d $TFLAG $SEMFLAG -a "$next_bench" "$Model" &
                PIDS[i]=$!
                BENCHS=("${BENCHS[@]:1}")
            fi
        fi
    done

    # Remove empty slots in the PIDS array
    PIDS=("${PIDS[@]}")

    # Ensure we always have NPROC running
    while [[ ${#PIDS[@]} -lt NPROC && ${#BENCHS[@]} -gt 0 ]]; do
        next_bench=${BENCHS[0]}  # Take the next benchmark
        echo "--> runFuzz @$next_bench"
        python -m semlearn -d $TFLAG $SEMFLAG -a "$next_bench" "$Model" &
        PIDS+=($!)
        BENCHS=("${BENCHS[@]:1}")  # Remove the first benchmark
    done

    sleep 10
done

echo "All benchmarks processed."
