#!/bin/bash
ROOT=`pwd`
BEN_DIR=`cd ../../Benchmarks && pwd`

###############################################################################
# ds-7b: default deepseek model
# 1    : 1 fuzzing instances a time
# -t   : 24h' running
###############################################################################
cd $BEN_DIR
./runAllFuzz.sh ds-7b 1 -t
cd -
