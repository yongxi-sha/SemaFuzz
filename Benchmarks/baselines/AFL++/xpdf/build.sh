

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=xpdf-4.05
Action=$1

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake ../$target
	make -j4
	cd -
		
	unset CC
	unset CXX
}


function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $benchDir/$benchName/$target.tar.gz -C $ROOT/	
	fi
	
	# alf-cc comliling: for fuzzing
	afl_compile
}


if [ "$Action" == "clean" ]; then
    rm -rf $target
    exit 0
fi

cd $ROOT
compile

cd $ROOT
