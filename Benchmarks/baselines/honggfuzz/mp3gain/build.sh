

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=mp3gain-1_6_2-src
Action=$1

function hgf_compile ()
{
	export CC="hfuzz-clang -fsanitize-coverage=trace-pc-guard"
	export CXX="hfuzz-clang++ -fsanitize-coverage=trace-pc-guard"

	cd $target
	make clean && make
	cd -

	unset CC
	unset CXX
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
	if [ ! -d "$ROOT/$target" ]; then
		unzip $benchDir/$benchName/$target.zip -d $target
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
