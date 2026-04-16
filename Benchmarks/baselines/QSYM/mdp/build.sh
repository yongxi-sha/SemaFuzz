#! /bin/bash

export ROOT=`pwd`
export target=mdp
export PATH=/usr/local/bin:$PATH
Action=$1

function install_deps ()
{
	apt-get install libncurses5-dev libncursesw5-dev -y
}

function afl_compile () {

	export CC="/AFLplusplus/afl-gcc-fast"
	export CXX="/AFLplusplus/afl-g++-fast"

	cd $ROOT/$target

	make clean && make CFLAGS="-D__FUZZING__"

	mv mdp $ROOT/mdp_instrumented

    unset CC
    unset CXX
}

function normal_compile(){

	export CC="gcc"
	export CXX="g++"

	cd $ROOT/$target

	make clean && make CFLAGS="-D__FUZZING__"

	mv mdp $ROOT/mdp_normal

    unset CC
    unset CXX
}

cd $ROOT
install_deps
afl_compile
cd $ROOT
normal_compile
cd $ROOT
