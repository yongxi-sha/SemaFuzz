#! /bin/bash

export ROOT=`pwd`
export target=uriparser
export PATH=/usr/local/bin:$PATH
Action=$1

function install_deps(){
	apt-get install libgtest-dev doxygen graphviz -y
	
	cd /usr/src/gtest
	cmake .
	make
}

function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a ..
	make clean && make
	
	export URIP_PATH=$ROOT/$target
	export URIP_LIBS=$ROOT/$target/build
	cd $ROOT/uripfuzz
	make clean && make

	mv $ROOT/uripfuzz/uripfuzz $ROOT/uripfuzz/uripfuzz_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target
	
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a ..
	make clean && make
	
	export URIP_PATH=$ROOT/$target
	export URIP_LIBS=$ROOT/$target/build
	cd $ROOT/uripfuzz
	make clean && make

	mv $ROOT/uripfuzz/uripfuzz $ROOT/uripfuzz/uripfuzz_normal

    unset CC
    unset CXX
}

cd $ROOT
install_deps
cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
