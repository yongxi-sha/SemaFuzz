

ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=mp4v2-2.1.3
EXE=mp4track

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $ROOT/build/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute Honggfuzz
cd fuzz
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE --list ___FILE___


