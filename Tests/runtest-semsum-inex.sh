#!/bin/bash

# TEST script for import and export semantic summary in json
# here, we use the result of symbolic-test for testing
TEST_DIR="symbolic-test/testcases"
LOG_DIR="inex-logs"

Action=$1

if ! command -v jq &> /dev/null; then
    apt -y install jq
fi

function clean() {
    echo "Cleaning up..."
    rm -f *.json
    echo "Cleaned successfully!"
    exit 0
}

function showtests() 
{
    ALLTEST=`ls $TEST_DIR/*.json`
    for file in $ALLTEST; do	
        base_name=$(basename "$file" .json)
        echo $base_name
    done
}

function run() 
{
	testName=$1
    mkdir -p $LOG_DIR

    ALLTEST=`ls $TEST_DIR/*.json`
    for file in $ALLTEST; do
   	
        base_name=$(basename "$file" .json)   
        if [[ "$testName" != "" && "$testName" != "$base_name" ]]; then
    		continue
		fi
        
        echo 
        echo "########## running test --> $base_name"   
        
		semcore -s $file -t json
		
		OUTPUT_FILE=$TEST_DIR/"export_$base_name.json"
		NORMALIZE_JSON="normalized_$base_name.json"
		
        jq -S . $OUTPUT_FILE > $NORMALIZE_JSON
        if diff -u $NORMALIZE_JSON $file > "$LOG_DIR/${base_name}_diff.log"; then
            echo "passed."
            echo
        else
            echo "$base_name failed. See $LOG_DIR/${base_name}_diff.log for details."
			cat "$LOG_DIR/${base_name}_diff.log"
            echo
        fi

        rm -f $OUTPUT_FILE
        rm -f $NORMALIZE_JSON
    done
}

# Check action argument and call the appropriate function
if [[ $Action == "clean" ]]; then
    clean
elif [[ $Action == "run" ]]; then
    run $2
else
    echo "Invalid action. Use 'clean' or 'run'."
    showtests
    exit 1
fi
