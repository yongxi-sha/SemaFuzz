#! /bin/bash

export ROOT=`pwd`
export target=jsonfuzz
export PATH=/usr/local/bin:$PATH
export JSON_PATH=$ROOT/json/include
Action=$1


function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $target


	make clean && make 

	mv $ROOT/$target/jsonfuzz $ROOT/jsonfuzz_instrumented
    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $target


	make clean && make 

	mv $ROOT/$target/jsonfuzz $ROOT/jsonfuzz_normal
    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
