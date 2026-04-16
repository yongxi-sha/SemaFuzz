#!/bin/bash

HONG_DIR="../../../Benchmarks/baselines/honggfuzz/"

for subdir in "$HONG_DIR"/*/; do

	covFile=cov_log.txt
    if [ -f "$subdir/$covFile" ]; then
        echo ">>>>>> $subdir already has $covFile, skipped ..."
        continue
    fi

	if [ -d "$subdir/fuzz/" ]; then
		rm -rf "$subdir/fuzz/"
	fi
	
	cd "$subdir" && echo "---> Fuzzing on $subdir..."
	python -m plog ./runFuzz.sh -c 4
	cd -
done





