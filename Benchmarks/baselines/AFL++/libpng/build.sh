

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=libpng
Action=$1

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	if [ ! -d "$ROOT/$target" ]; then 
		git clone https://github.com/pnggroup/libpng.git
	fi
	
	cd $target
	./configure
	make clean && make
	cd -

	export PNG_PATH=$ROOT/$target
	export PNG_LIBS=$ROOT/$target/.libs
	cp $benchDir/$benchName/pngfuzz $ROOT/ -rf
	cd $ROOT/pngfuzz
	make clean && make
	cd $ROOT/
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
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
