

export ROOT=`pwd`
export target=pycdc
Action=$1
BINARY=pycdc

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -w"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake ../$target
	make -j4
	
	extract-bc $BINARY && mv $BINARY.bc $ROOT/
	
	cd $ROOT
	export ONLY_CG="true"
	wpa -fspta -dump-callgraph $BINARY.bc 
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	cd $ROOT
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake ../$target
	make
	cd $ROOT
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

	if [ ! -d "$ROOT/$target" ]; then
		git clone https://github.com/zrax/pycdc.git
	fi
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		sym_exe $BINARY functions_list
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
