#! /bin/bash

export ROOT=`pwd`
export target=sablot-1.0.3
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $target.tar.gz 
	fi

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	./configure --enable-shared=no
	make CXXFLAGS="-Wno-reserved-user-defined-literal"

	mv $ROOT/$target/src/command/sabcmd $ROOT/sabcmd_instrumented

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

	./configure --enable-shared=no
	make CXXFLAGS="-Wno-reserved-user-defined-literal"

	mv $ROOT/$target/src/command/sabcmd $ROOT/sabcmd_normal

    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
