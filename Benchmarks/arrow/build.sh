

export ROOT=`pwd`
export target=arrow-apache-arrow-19.0.1
Action=$1

function install_deps() {
    ninjacmd=$(which ninja)
    if [ -z "$ninjacmd" ]; then
        apt-get install -y ninja-build
    fi

    if [ ! -d "/usr/local/include/xsimd" ]; then
        git clone https://github.com/xtensor-stack/xsimd.git
        cd xsimd
        mkdir build && cd build
        cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
        make && make install
        cd ../.. 
        rm -rf xsimd
    fi

	apt-get install rapidjson-dev
}

function svf_run ()
{
	module=$1
	path=$2

	extract-bc $path/$module && cp $path/$module.bc ./

	export ONLY_CG="true"
	#wpa -ander -dump-callgraph $module.bc
}

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

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
	cmake --build .
	
	cd $ROOT/
	svf_run libarrow.so $ROOT/$target/cpp/build/debug
}

function afl_compile ()
{
	delshm.sh
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

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
	unset FUNCTIONS_LIST
	cd -

	export ARROW_PATH="$ROOT/$target/cpp/src"
	export ARROW_BUILD="$ROOT/$target/cpp/build/src"
	export ARROW_LIB="$ROOT/$target/cpp/build/debug"
	cd $ROOT/arrowfuzz
	make clean && make
	cd -
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
	install_deps

	if [ ! -d "$ROOT/$target" ]; then
		tar -zxvf $target.tar.gz
	fi
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		sym_exe libarrow.so functions_list
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
