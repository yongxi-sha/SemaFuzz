#!/bin/bash

SOURCE_DIR="$(pwd)/../../Benchmarks/baselines/AFL++"

if [ ! -d "$SOURCE_DIR" ]; then
	echo "Source dir doesn't exist"
	exit 1
fi

for subdir in "$SOURCE_DIR"/*/; do
	folder_name=$(basename "$subdir")
	fuzz_dir="$subdir/fuzz"
	mkdir -p "$folder_name"
	if [ -f "$fuzz_dir/perf_periodic.txt" ]; then
		cp "$fuzz_dir/perf_periodic.txt" "$folder_name/"
		echo "copied: $fuzz_dir/perf_periodic.txt -> $folder_name/"
	else
		echo "No file found."
	fi
done	
