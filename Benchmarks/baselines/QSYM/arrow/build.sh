#! /bin/bash

export ROOT=`pwd`
export target=arrow-apache-arrow-19.0.1
export PATH=/usr/local/bin:$PATH
Action=$1

function install_deps() {
    ninjacmd=$(which ninja)
    if [ -z "$ninjacmd" ]; then
        apt-get install -y ninja-build
    fi
    if [ ! -d "/usr/local/include/xsimd" ]; then
        git clone https://github.com/xtensor-stack/xsimd.git
        cd xsimd
        mkdir build && cd build
        cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
        make && make install
        cd ../..
        rm -rf xsimd
    fi
	apt-get install rapidjson-dev
}

function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"
	cd $target/cpp
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake .. -GNinja \
    	-DCMAKE_BUILD_TYPE=Release \
    	-DCMAKE_INSTALL_PREFIX=/usr/local \
        -DARROW_SIMD_LEVEL=NONE \
        -DARROW_RUNTIME_SIMD_LEVEL=NONE \
        -DARROW_JSON=ON \
        -DARROW_CSV=ON \
        -DARROW_IPC=ON  
	cmake --build . -- -j1
	export ARROW_PATH="$ROOT/$target/cpp/src"
	export ARROW_BUILD="$ROOT/$target/cpp/build/src"
	export ARROW_LIB="$ROOT/$target/cpp/build/release"
	cd $ROOT/arrowfuzz
	make clean && make
	mv arrowfuzz $ROOT/arrowfuzz/arrowfuzz_instrumented
        unset CC
        unset CXX
}

function normal_compile(){
	export CC="gcc"
	export CXX="g++"
	cd $target/cpp
        if [ -d "build" ]; then rm -rf build; fi
        mkdir build && cd build
        cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
	-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=gold" \
	-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold" \
        -DARROW_SIMD_LEVEL=NONE \
        -DARROW_RUNTIME_SIMD_LEVEL=NONE \
        -DARROW_JSON=ON \
        -DARROW_CSV=ON \
        -DARROW_IPC=ON
        cmake --build . -- -j1
        export ARROW_PATH="$ROOT/$target/cpp/src"
        export ARROW_BUILD="$ROOT/$target/cpp/build/src"
        export ARROW_LIB="$ROOT/$target/cpp/build/release"
        cd $ROOT/arrowfuzz
        make clean && make
	mv arrowfuzz $ROOT/arrowfuzz/arrowfuzz_normal
        unset CC
        unset CXX
}

cd $ROOT
install_deps
afl_compile
cd $ROOT
normal_compile
cd $ROOT
