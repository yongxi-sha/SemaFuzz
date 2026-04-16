

export ROOT=`pwd`
export target=libpng
Action=$1

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

	if [ -d "$ROOT/$target" ]; then rm -rf $ROOT/$target; fi
	git clone https://github.com/pnggroup/libpng.git

	cd $target
	./configure
	make clean && make
	cd -
	
	svf_run libpng16.so $ROOT/$target/.libs
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	if [ -d "$ROOT/$target" ]; then rm -rf $ROOT/$target; fi
	git clone https://github.com/pnggroup/libpng.git

	cd $target
	./configure
	make clean && make
	cd -
	unset FUNCTIONS_LIST

	export FUNCOVERAGE_OFF="TRUE"
	export PNG_PATH=$ROOT/$target
	export PNG_LIBS=$ROOT/$target/.libs
	cd $ROOT/pngfuzz
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
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		sym_exe libpng16.so functions_list
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
