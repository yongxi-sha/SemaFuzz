#!/bin/bash

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=mdp
BINARY=mdp
Action=$1

function hgf_compile ()
{
	export CC="hfuzz-clang -fsanitize-coverage=trace-pc-guard"
	export CXX="hfuzz-clang++ -fsanitize-coverage=trace-pc-guard"

	cd $ROOT/$target
	make clean && make CFLAGS="-D__FUZZING__"
	cd $ROOT
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $benchDir/$target/"$target".tar
	fi
	
	hgf_compile
}

if [ "$Action" == "clean" ]; then
	rm -rf $target
	exit 0
fi

cd $ROOT
compile

cd $ROOT
