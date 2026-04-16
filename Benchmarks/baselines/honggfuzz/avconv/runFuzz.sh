ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=libav-12.3
EXE=avconv

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $TARGET/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute honggfuzz
cd fuzz
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE -i ___FILE___
