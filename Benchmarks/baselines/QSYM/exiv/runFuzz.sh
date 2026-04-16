#!/bin/bash

export ROOT=`pwd`

SEEDS=$ROOT/seeds.tar

cd $ROOT
# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
tar -xvf $SEEDS -C fuzz/in/

SEED="fuzz/in"
OUTPUT="fuzz/out"
AFL_CMDLINE="$ROOT/exiv2_instrumented"
QSYM_CMDLINE="$ROOT/exiv2_normal"
CMD="@@"

/AFLplusplus/afl-fuzz -t 1000+ -m none -M afl-master -i $SEED -o $OUTPUT -- $AFL_CMDLINE $CMD &
sleep 30
/AFLplusplus/afl-fuzz -t 1000+ -m none -S afl-slave -i $SEED -o $OUTPUT -- $AFL_CMDLINE $CMD &
sleep 30
while [ ! -f $OUTPUT/afl-slave/fuzzer_stats ]
do
	sleep 2
	echo "no fuzzer_stats sleep 2"
done
/workdir/qsym/bin/run_qsym_afl.py -a afl-slave -o $OUTPUT -n qsym -- $QSYM_CMDLINE $CMD &
sleep infinity