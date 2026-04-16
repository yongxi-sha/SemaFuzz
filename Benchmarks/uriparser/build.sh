

export ROOT=`pwd`
export target=uriparser
Action=$1

function install_gtest ()
{
	apt-get install libgtest-dev

	if [ -f "/usr/src/gtest/libgtest.a" ]; then
		return
	fi

	cd usr/src/gtest
	cmake .
	make
	cd -
}

function svf_run ()
{
	module=$1
	path=$2

	extract-bc $path/$module && cp $path/$module.bc ./

	export ONLY_CG="true"
	wpa -fspta -dump-callgraph $module.bc
}

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	
	cmake -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a ../$target
	make -j4
	cd -
		
	svf_run liburiparser.so build
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake -DCMAKE_BUILD_TYPE=Release -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a ../$target
	make
	cd -

	export URIP_PATH=$ROOT/$target
	export URIP_LIBS=$ROOT/build
	cd $ROOT/uripfuzz
	make clean && make
	cd $ROOT/
}

function sym_exe ()
{
	MODULE=$1

	BC=$MODULE.bc
	FL=$2
	symrun $BC $FL
	jq -S . symbolic_summary.json > normalized_symbolic_summary.json
	mv normalized_symbolic_summary.json symbolic_summary.json
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	apt-get install doxygen

	if [ ! -d "$ROOT/$target" ]; then
		git clone https://github.com/uriparser/uriparser.git
	fi
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		sym_exe liburiparser.so functions_list
	else
		afl_compile
	fi

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
