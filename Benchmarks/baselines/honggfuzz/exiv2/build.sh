

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=exiv2-0.28.3
Action=$1

function hgf_compile ()
{
	export CC="hfuzz-clang -fsanitize-coverage=trace-pc-guard"
	export CXX="hfuzz-clang++ -fsanitize-coverage=trace-pc-guard"

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $benchDir/$benchName/$target.tar.gz -C $ROOT/
	fi	

	if [ ! -d "$ROOT/inih" ]; then
		git clone https://github.com/benhoyt/inih.git
	fi

	cd $target
	cmake -S . -B build -G "Unix Makefiles" -DCXX_FILESYSTEM_STDCPPFS_NEEDED=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON\
		-Dinih_inireader_INCLUDE_DIR=$ROOT/inih/cpp -Dinih_inireader_LIBRARY=$ROOT/inih/cpp/INIReader.cpp
	cmake --build build
	cd -

	unset CC
	unset CXX
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	hgf_compile
}


if [ "$Action" == "clean" ]; then
	rm -rf $target fuzz inih
	exit 0
fi

cd $ROOT
compile

cd $ROOT
