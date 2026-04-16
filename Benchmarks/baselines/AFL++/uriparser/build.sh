

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=uriparser
Action=$1

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a ../$target
	make
	cd -

	export FUNCOVERAGE_OFF="TRUE"
	export URIP_PATH=$ROOT/$target
	export URIP_LIBS=$ROOT/build

	cp $benchDir/$benchName/uripfuzz $ROOT/ -rf
	cd $ROOT/uripfuzz
	make clean && make
	cd $ROOT/
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	if [ ! -d "$ROOT/$target" ]; then
		git clone https://github.com/uriparser/uriparser.git
	fi
	
	# alf-cc comliling: for fuzzing	
	afl_compile

	unset CC
	unset CXX
}

if [ "$Action" == "clean" ]; then
	rm -rf build
	rm -rf $target
	
	exit 0
fi

cd $ROOT
compile

cd $ROOT
