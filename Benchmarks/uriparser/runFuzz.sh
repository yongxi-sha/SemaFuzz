ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd .. && pwd`
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

# Change to fuzz directory and execute AFL++
export LD_LIBRARY_PATH=$ROOT/build:$LD_LIBRARY_PATH

cd fuzz
afl-system-config
afl-fuzz -t 5000 -i in/ -o out -- ./$EXE @@
