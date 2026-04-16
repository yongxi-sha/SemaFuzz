#! /bin/bash

export ROOT=`pwd`
export target=tcpdump
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	make clean
	autoreconf -ivf
	./configure
	make clean && make

	mv $ROOT/$target/tcpdump $ROOT/tcpdump_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	make clean
	autoreconf -ivf
	./configure
	make clean && make

	mv $ROOT/$target/tcpdump $ROOT/tcpdump_normal

    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
