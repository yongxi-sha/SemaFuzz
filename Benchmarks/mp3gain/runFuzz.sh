

ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd .. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=mp3gain-1_6_2-src
EXE=mp3gain

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $TARGET/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute AFL++
cd fuzz
afl-system-config
afl-fuzz -i in/ -o out -- ./$EXE @@

