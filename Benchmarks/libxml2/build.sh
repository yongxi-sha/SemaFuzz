

export ROOT=`pwd`
export target=libxml2-2.13.0
export FUZZ_HOME="$ROOT/fuzz"
Action=$1

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"
	
	cd $target
	./configure --enable-shared=no --prefix="$FUZZ_HOME"
	make
	cd -

	extract-bc $target/xmllint
	ls -l $target/xmllint*

	cp $target/xmllint.bc ./

	export ONLY_CG="true"
	wpa -fspta -dump-callgraph xmllint.bc
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNC_EXCEP="conftest_c_main xmlcatalog_c_main"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	cd $target
	./configure --enable-shared=no --prefix="$FUZZ_HOME"
	make
	cd -
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH
	
	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $target.tar.xz
	fi

	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing
		afl_compile

		# function summarization
		symrun xmllint.bc functions_list
		jq -S . symbolic_summary.json > normalized_symbolic_summary.json
		mv normalized_symbolic_summary.json symbolic_summary.json
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
