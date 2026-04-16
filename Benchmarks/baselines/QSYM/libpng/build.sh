#! /bin/bash

export ROOT=`pwd`
export target=libpng
export PATH=/usr/local/bin:$PATH

Action=$1


function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	if [ -d "$ROOT/$target" ]; then rm -rf $ROOT/$target; fi
	git clone https://github.com/pnggroup/libpng.git

	cd $target

	./configure
	
	make clean && make 
	export FUNCOVERAGE_OFF="TRUE"
	export PNG_PATH=$ROOT/$target
	export PNG_LIBS=$ROOT/$target/.libs
	cd $ROOT/pngfuzz
	make clean && make
	mv $ROOT/$target/libfuzz $ROOT/libfuzz_instrumented
    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	if [ -d "$ROOT/$target" ]; then rm -rf $ROOT/$target; fi
	git clone https://github.com/pnggroup/libpng.git

	cd $target

	./configure
	make clean && make 
	export PNG_PATH=$ROOT/$target
	export PNG_LIBS=$ROOT/$target/.libs
	cd $ROOT/pngfuzz
	make clean && make
	mv $ROOT/$target/libfuzz $ROOT/libfuzz_normal
    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
#normal_compile
cd $ROOT
