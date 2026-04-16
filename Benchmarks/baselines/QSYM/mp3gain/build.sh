#! /bin/bash

export ROOT=`pwd`
export target=mp3gain
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	make clean && make

	mv mp3gain $ROOT/mp3gain_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	make clean && make

	mv mp3gain $ROOT/mp3gain_normal

    unset CC
    unset CXX
}

cd $ROOT
install_deps
afl_compile
cd $ROOT
normal_compile
cd $ROOT
