

export ROOT=`pwd`
export target=mp3gain-1_6_2-src
Action=$1

function install_deps ()
{
    apt-get install libmpg123-dev
}

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"
	
	cd $target
	make clean && make
	cd -

	extract-bc $target/mp3gain

	cp $target/mp3gain.bc ./
	export ONLY_CG="true"
	wpa -fspta -dump-callgraph mp3gain.bc
}

function afl_compile ()
{
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
	
	if [ ! -d "$ROOT/$target" ]; then
		unzip $target.zip -d $target
	fi

	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing
		afl_compile

		# function summarization
		sym_exe mp3gain functions_list
	else
		afl_compile
	fi

	unset CC
	unset CXX
}


if [ "$Action" == "clean" ]; then
        rm -rf $target
	rm -rf $FUZZ
	rm -rf callgraph*
	rm -rf functions_list
	rm -rf *.bc

        exit 0
fi

cd $ROOT
compile

cd $ROOT
