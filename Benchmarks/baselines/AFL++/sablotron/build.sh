

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=sablot-1.0.3
Action=$1

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	cd $target
	./configure --enable-shared=no
	make CXXFLAGS="-Wno-reserved-user-defined-literal"
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
