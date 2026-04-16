

ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=libxml2-2.13.0
EXE=xmllint

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $TARGET/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute Honggfuzz
cd fuzz
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE ___FILE___

