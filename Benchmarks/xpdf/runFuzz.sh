

ROOT=`pwd`
benchName=$(basename "$ROOT")

BENCHMARKS=`cd .. && pwd`
SEEDS=$BENCHMARKS/$benchName/seeds.tar

TARGET=xpdf-4.05
EXE=pdfinfo

# Ensure directories exist
mkdir -p fuzz/in
mkdir -p fuzz/out

# Copy the executable and input samples
cp $ROOT/build/xpdf/$EXE fuzz/
tar -xvf $SEEDS -C fuzz/in/

# Change to fuzz directory and execute Honggfuzz
cd fuzz
afl-system-config
afl-fuzz -i in/ -o out -- ./$EXE @@
