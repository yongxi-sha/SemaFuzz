#!/bin/bash

# Directory for logs and outputs
LOG_DIR="logs"
OUTPUT="demo.bc"
OUTPUT_FILE="symbolic_summary.json"
NORMALIZE_JSON="normalized_$OUTPUT_FILE"
CC=clang
TESTPATH="testcases"

Action=$1

if ! command -v jq &> /dev/null; then
    apt -y install jq
fi

function clean() {
    echo "Cleaning up..."
    rm -rf $LOG_DIR $OUTPUT_FILE $OUTPUT functions_list *.json
    echo "Cleaned successfully!"
    exit 0
}

export SVF_STAT_OFF=1
function run() {
	testName=$1
    mkdir -p $LOG_DIR

    ALLTEST=`ls $TESTPATH/*.c`
    for file in $ALLTEST; do
   	
        base_name=$(basename "$file" .c)   
        if [[ "$testName" != "" && "$testName" != "$base_name" ]]; then
    		continue
		fi
        
        echo 
        echo "########## running test --> $base_name"   
        
	    rm -f $OUTPUT_FILE $NORMALIZE_JSON
        $CC -emit-llvm -c "$file" -o "$OUTPUT"  > "$LOG_DIR/${base_name}.log" 2>&1
        symrun $OUTPUT > "$LOG_DIR/${base_name}.log" 2>&1
        if [ $? -ne 0 ]; then
            echo "Compilation failed for $file. See $LOG_DIR/${base_name}.log for details."
            echo
            continue
        fi

        jq -S . $OUTPUT_FILE > $NORMALIZE_JSON
        if diff -u $NORMALIZE_JSON "$TESTPATH/${base_name}.json" > "$LOG_DIR/${base_name}_diff.log"; then
            echo "passed."
            echo
        else
            echo "$base_name failed. See $LOG_DIR/${base_name}_diff.log for details."
			cat "$LOG_DIR/${base_name}_diff.log"
            echo
        fi
    done
}

# Check action argument and call the appropriate function
if [[ $Action == "clean" ]]; then
    clean
elif [[ $Action == "run" ]]; then
    run $2
else
    echo "Invalid action. Use 'clean' or 'run'."
    exit 1
fi
