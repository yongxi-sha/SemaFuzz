#!/bin/bash


AFL_DIR="../../../Benchmarks/baselines/AFL++"

for subdir in "$AFL_DIR"/*/; do

	covFile=cov_log.txt
    if [ -f "$subdir/$covFile" ]; then
        echo ">>>>>> $subdir already has $covFile, skipped ..."
        continue
    fi

	if [ -d "$subdir/fuzz/" ]; then
		rm -rf "$subdir/fuzz/"
	fi

	folder_dir=$(basename "$subdir")

	cd "$subdir" && echo "---> Fuzzing on $subdir..."
	python -m plog ./runFuzz.sh -c 4
	cd -
done





