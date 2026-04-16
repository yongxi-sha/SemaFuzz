#!/bin/bash


for file in *.json; do
    if [[ "$file" != "export_symbolic_summary.json" && "$file" != "symbolic_summary.json" ]]; then
        mv "$file" ../../Tests/dynamic_mode/jsons/
    fi
done


for file in *.dot; do
    if [[ "$file" != "callgraph_final.dot" && "$file" != "callgraph_initial.dot" ]]; then
        rm "$file"
    fi
done

