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
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute AFL++
cd fuzz
export AFL_MAP_SIZE=361024
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/libstdc++.so.6:$ROOT/arrow-apache-arrow-19.0.1/cpp/build/debug:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
afl-system-config
afl-fuzz -t 5000 -i in/ -o out -- ./$EXE @@
