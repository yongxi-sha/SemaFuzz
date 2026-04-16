ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=arrowfuzz
EXE=arrowfuzz

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $TARGET/$EXE fuzz/
cp arrow-apache-arrow-19.0.1/cpp/build/debug/libarrow.so.1900 fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute Honggfuzz
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/libstdc++.so.6:$ROOT/arrow-apache-arrow-19.0.1/cpp/build/debug:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
cd fuzz
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE ___FILE___

