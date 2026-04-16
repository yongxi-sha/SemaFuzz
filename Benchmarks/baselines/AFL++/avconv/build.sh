

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export target=libav-12.3
export benchDir=`cd ../../../ && pwd`
Action=$1

function afl_compile ()
{
	export CC="afl-cc"
	export FUNCOVERAGE_OFF="TRUE"

	if [ -d "$ROOT/$target" ]; then rm -rf $ROOT/$target; fi
	tar -xvf $benchDir/$benchName/$target.tar.gz -C $ROOT/
	cp Makefile $ROOT/$target/

	cd $target
	./configure --disable-yasm --disable-shared --cc=$CC --extra-ldflags="-fPIC -lshmQueue"
	make clean && make
	cd -
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
