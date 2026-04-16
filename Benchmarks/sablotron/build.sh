

export ROOT=`pwd`
export target=sablot-1.0.3
Action=$1

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

	cd $target
	./configure --enable-shared=no
	make clean && make CXXFLAGS="-Wno-reserved-user-defined-literal"
	cd -

	extract-bc $target/src/command/sabcmd
	ls -l $target/src/command/sabcmd*

	cp $target/src/command/sabcmd.bc ./

	export ONLY_CG="true"
	wpa -fspta  -dump-callgraph sabcmd.bc
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	cd $target
	./configure --enable-shared=no
	make CXXFLAGS="-Wno-reserved-user-defined-literal"
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
		tar -xvf $target.tar.gz 
	fi

	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile

		# alf-cc comliling: for fuzzing
		afl_compile

		# function summarization
		sym_exe sabcmd functions_list
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
