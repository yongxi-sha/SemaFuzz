

export ROOT=`pwd`
export target=jsonfuzz
Action=$1

function wllvm_compile ()
{
	export JSON_PATH=$ROOT/json/include
	
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

	cd $target
	make clean && make
	
	export ONLY_CG="true"
	extract-bc jsonfuzz && mv jsonfuzz.bc ../
	cd -

	wpa -fspta -dump-callgraph jsonfuzz.bc 
}

function afl_compile ()
{
	export JSON_PATH=$ROOT/json/include
	
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	cd $target
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

	if [ ! -d "$ROOT/json" ]; then
		git clone https://github.com/nlohmann/json.git
	fi
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		sym_exe jsonfuzz functions_list
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
