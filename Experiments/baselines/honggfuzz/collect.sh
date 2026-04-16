#!/bin/bash

SOURCE_DIR="$(pwd)/../../Benchmarks/baselines/honggfuzz"

if [ ! -d "$SOURCE_DIR" ]; then
	echo "Source dir doesn't exist"
	exit 1
fi

for subdir in "$SOURCE_DIR"/*/; do
	folder_name=$(basename "$subdir")
	mkdir -p "$folder_name"
	if [ -f "$subdir/cov_log.txt" ]; then
		cp "$subdir/cov_log.txt" "$folder_name/"
		echo "copied: $subdir/cov_log.txt -> $folder_name/"
	else
		echo "No file found."
	fi
done	
