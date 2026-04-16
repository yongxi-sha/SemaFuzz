#!/bin/bash

bench="$1"
cmd="$2"
para="--oldxml10"

# Enter the fuzz directory
cd "$bench/fuzz" || exit 1

# Store all files from out/* in an array
# (If out/* is empty, this array will also be empty.)
allCorpus=( out/default/queue/* )

# Get total number of files
total=${#allCorpus[@]}

index=0
for file in "${allCorpus[@]}"; do
    echo "[$index/$total] --> $file"
    ./$cmd $para "$file"
    (( index++ ))
done
