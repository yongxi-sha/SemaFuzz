ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd .. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=pngfuzz
EXE=pngfuzz

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $TARGET/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute AFL++
cd fuzz
afl-system-config
afl-fuzz -t 5000 -i in/ -o out -- ./$EXE @@
