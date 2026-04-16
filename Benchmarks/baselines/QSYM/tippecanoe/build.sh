#! /bin/bash

export ROOT=`pwd`
export target=tippecanoe
export PATH=/usr/local/bin:$PATH
Action=$1

function install_deps(){
	sudo apt-get install libsqlite3-dev zlib1g-dev -y
}

function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	make clean && make CXXFLAGS="-std=gnu++17 -O3 -DNDEBUG -g" \
  	CFLAGS="-O3 -DNDEBUG -g"

	mv $ROOT/$target/tippecanoe $ROOT/tippecanoe_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	make clean && make CXXFLAGS="-std=gnu++17 -O3 -DNDEBUG -g" \
  	CFLAGS="-O3 -DNDEBUG -g"

	mv $ROOT/$target/tippecanoe $ROOT/tippecanoe_normal

    unset CC
    unset CXX
}

cd $ROOT
install_deps
afl_compile
cd $ROOT
normal_compile
cd $ROOT
