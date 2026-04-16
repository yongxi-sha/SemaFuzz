

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export target=arrow-apache-arrow-19.0.1
export benchDir=`cd ../../../ && pwd`
Action=$1

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	cd $target/cpp
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	
	cmake .. -GNinja \
    	-DCMAKE_BUILD_TYPE=Debug \
    	-DCMAKE_INSTALL_PREFIX=/usr/local \
    	-DARROW_JSON=ON \
    	-DARROW_CSV=ON \
    	-DARROW_IPC=ON \
    	-DARROW_IO=ON
	cmake --build . -- -j1
	cd -

	export ARROW_PATH="$ROOT/$target/cpp/src"
	export ARROW_BUILD="$ROOT/$target/cpp/build/src"
	export ARROW_LIB="$ROOT/$target/cpp/build/debug"
	cd $ROOT/arrowfuzz
	make clean && make
	cd -
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $benchDir/$benchName/$target.tar.gz
	fi

	if [ ! -d "$ROOT/arrowfuzz" ]; then
		cp $benchDir/$benchName/arrowfuzz ./ -rf
	fi
	
	# alf-cc comliling: for fuzzing
	afl_compile

	unset CC
	unset CXX
}


if [ "$Action" == "clean" ]; then
	rm -rf $target
	exit 0
fi

cd $ROOT
compile

cd $ROOT
