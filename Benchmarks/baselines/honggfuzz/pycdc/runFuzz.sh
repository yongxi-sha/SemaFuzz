

ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=pycdc
EXE=pycdc

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $ROOT/build/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute AFL++
cd fuzz
afl-system-config
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE ___FILE___

