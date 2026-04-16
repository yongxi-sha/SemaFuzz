#! /bin/bash

export ROOT=`pwd`
export target=libav-12.3
export PATH=/usr/local/bin:$PATH
Action=$1


function afl_compile () {

    export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"
    cd $ROOT/$target
    if [ -d "build" ]; then rm -rf build; fi
    mkdir build && cd build
    ../configure --disable-yasm --disable-shared --cc=$CC
    make clean && make -j$(nproc)
    mv avconv $ROOT/avconv_instrumented	
    unset CC
    unset CXX
}

function normal_compile(){
    export CC="gcc"
    export CXX="g++"
    cd $ROOT/$target
    if [ -d "build" ]; then rm -rf build; fi
    mkdir build && cd build
    ../configure --disable-yasm --disable-shared --cc=$CC
    make clean && make -j$(nproc)
    mv avconv $ROOT/avconv_normal
    unset CC
    unset CXX
}

cd $ROOT
afl_compile
cd $ROOT
normal_compile
cd $ROOT
