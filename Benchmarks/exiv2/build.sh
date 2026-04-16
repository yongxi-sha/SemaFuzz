

export ROOT=`pwd`
export target=exiv2-0.28.3
export FUZZ_HOME="$ROOT/fuzz"
Action=$1


function install_gcc9 ()
{
	add-apt-repository ppa:ubuntu-toolchain-r/test
	apt install gcc-9 g++-9
	
	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60
	update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 60

	gcc --version
}

function install_deps ()
{
	apt install zlib1g-dev libbrotli-dev libinih-dev libexpat1-dev libgtest-dev
}

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

	if [ -d "$ROOT/$target" ]; then
		rm -rf $ROOT/$target
	fi
	tar -xvf $target.tar.gz

	if [ ! -d "$ROOT/inih" ]; then
		git clone https://github.com/benhoyt/inih.git
	fi
	
	cd $target
	cmake -S . -B build -G "Unix Makefiles" -DCXX_FILESYSTEM_STDCPPFS_NEEDED=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON\
		-Dinih_inireader_INCLUDE_DIR=$ROOT/inih/cpp -Dinih_inireader_LIBRARY=$ROOT/inih/cpp/INIReader.cpp
	cmake --build build
	cd -

	extract-bc $target/build/bin/exiv2

	cp $target/build/bin/exiv2.bc ./

	export ONLY_CG="true"
	wpa -fspta -dump-callgraph exiv2.bc
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue -lstdc++fs"
	export CXX="afl-c++ -fPIC -lshmQueue -lstdc++fs"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	if [ -d "$ROOT/$target" ]; then
		rm -rf $ROOT/$target
	fi
	tar -xvf $target.tar.gz

	if [ ! -d "$ROOT/inih" ]; then
		git clone https://github.com/benhoyt/inih.git
	fi

	cd $target
	cmake -S . -B build -G "Unix Makefiles" -DCXX_FILESYSTEM_STDCPPFS_NEEDED=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON\
		-Dinih_inireader_INCLUDE_DIR=$ROOT/inih/cpp -Dinih_inireader_LIBRARY=$ROOT/inih/cpp/INIReader.cpp
	cmake --build build
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

	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing
		afl_compile

		# function summarization
		sym_exe exiv2 functions_list
	else
		afl_compile
	fi

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
