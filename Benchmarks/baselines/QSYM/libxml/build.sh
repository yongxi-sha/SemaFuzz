#! /bin/bash

export ROOT=`pwd`
export target=libxml2-2.13.0
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $target
	./configure --enable-shared=no

	make clean && make 

	mv xmllint $ROOT/xmllint_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $target
	./configure --enable-shared=no

	make clean && make 

	mv xmllint $ROOT/xmllint_normal

    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
