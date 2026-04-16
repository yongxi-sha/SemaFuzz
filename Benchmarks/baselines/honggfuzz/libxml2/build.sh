

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=libxml2-2.13.0
Action=$1

function hgf_compile ()
{
	export CC="hfuzz-clang -fsanitize-coverage=trace-pc-guard"
	export CXX="hfuzz-clang++ -fsanitize-coverage=trace-pc-guard"

	cd $target
	./configure --enable-shared=no
	make
	cd -
		
	unset CC
	unset CXX
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $benchDir/$benchName/$target.tar.xz -C $ROOT/
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
