#! /bin/bash

export ROOT=`pwd`
export target=xpdf-4.05
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf xpdf-4.05.tar.gz 
	fi

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake ..
	make

	mv $ROOT/$target/build/xpdf/pdfinfo $ROOT/pdfinfo_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf xpdf-4.05.tar.gz 
	fi

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake ..
	make

	mv $ROOT/$target/build/xpdf/pdfinfo $ROOT/pdfinfo_normal

    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
