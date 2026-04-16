#! /bin/bash

export ROOT=`pwd`
export target=mp4v2-2.1.3
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $target.tar.gz
	fi

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake .. && make

	mv mp4art $ROOT/mp4art_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $target.tar.gz
	fi

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake .. && make

	mv mp4art $ROOT/mp4art_normal

    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
