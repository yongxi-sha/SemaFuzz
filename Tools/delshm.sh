#!/bin/bash

# Get the keys and IDs of the shared memory segments
keys_and_ids=$(ipcs -m | awk '$1~/[0-9]+/ {print $1, $2}')

# Print keys for debugging
echo "$keys_and_ids"

# Loop over each key and ID
while read -r key id
do
    if [ "$key" != "0x00000000" ]; then
        if [ -n "$id" ]; then
            echo "ipcrm -m $id"
            ipcrm -m "$id"
        fi
    fi
done <<< "$keys_and_ids"
