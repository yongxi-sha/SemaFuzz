ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd ../../.. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

target=uripfuzz
EXE=uripfuzz

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out
cp build/liburiparser.so.1 fuzz/

# Copy the executable and input samples
cp $target/$EXE fuzz/
mkdir seeds && tar -xvf $SEEDS -C seeds
mv seeds/* fuzz/in/

export LD_LIBRARY_PATH=$ROOT/build:$LD_LIBRARY_PATH

# Change to fuzz directory and execute Honggfuzz
cd fuzz
honggfuzz -i in/ -o out --timeout 10 --rlimit_rss 1024 -- ./$EXE ___FILE___
