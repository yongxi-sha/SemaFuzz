#! /bin/bash

export ROOT=`pwd`
export target=exiv2-0.28.3
export PATH=/usr/local/bin:$PATH
Action=$1

function install_deps ()
{
	apt install zlib1g-dev libbrotli-dev libinih-dev libexpat1-dev libgtest-dev -y
}

function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast  -lstdc++fs"
	export CXX="/AFLplusplus/afl-g++-fast  -lstdc++fs"
    if [ ! -d "$ROOT/inih" ]; then
		git clone https://github.com/benhoyt/inih.git
	fi
	cd $target
	if [ -d "build" ]; then rm -rf build; fi

	cmake -S . -B build -G "Unix Makefiles" -DCXX_FILESYSTEM_STDCPPFS_NEEDED=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON\
		-Dinih_INCLUDE_DIR=$ROOT/inih -Dinih_LIBRARY=$ROOT/inih/libinih.a   -Dinih_inireader_INCLUDE_DIR="$ROOT/inih/cpp" \
        -Dinih_inireader_LIBRARY="$ROOT/inih/libinih_inireader.a"
	cmake --build build

	mv $ROOT/$target/build/bin/exiv2 $ROOT/exiv2_instrumented
        unset CC
        unset CXX
}

function normal_compile(){
	export CC="gcc -lstdc++fs"
	export CXX="g++ -lstdc++fs"
        if [ ! -d "$ROOT/inih" ]; then
		git clone https://github.com/benhoyt/inih.git
	fi
	cd $target
	if [ -d "build" ]; then rm -rf build; fi

	cmake -S . -B build -G "Unix Makefiles" -DCXX_FILESYSTEM_STDCPPFS_NEEDED=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON\
		-Dinih_INCLUDE_DIR=$ROOT/inih -Dinih_LIBRARY=$ROOT/inih/libinih.a   -Dinih_inireader_INCLUDE_DIR="$ROOT/inih/cpp" \
        -Dinih_inireader_LIBRARY="$ROOT/inih/libinih_inireader.a"
	cmake --build build

	mv $ROOT/$target/build/bin/exiv2 $ROOT/exiv2_normal
        unset CC
        unset CXX
}

cd $ROOT
install_deps
afl_compile
cd $ROOT
normal_compile
cd $ROOT
