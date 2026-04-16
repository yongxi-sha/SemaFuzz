

export ROOT=`pwd`
export target=xpdf-4.05
Action=$1

function svf_run ()
{
	module=$1
	path=$2

	extract-bc $path/$module && cp $path/$module.bc ./

	export ONLY_CG="true"
	wpa -fspta -dump-callgraph $module.bc #&& mv callgraph_final.dot $module"_callgraph_final.dot"
}

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	
	cmake ../$target
	make -j4
	cd -
		
	#svf_run pdftotext build/xpdf
	svf_run pdfinfo build/xpdf
	#svf_run pdftopng  build/xpdf
}

function afl_compile ()
{
	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCTIONS_LIST="$ROOT/functions_list"

	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build

	cmake ../$target
	make
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
	#rm symbolic_summary.json
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf xpdf-4.05.tar.gz 
	fi
	
	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing	
		afl_compile

		# function summarization
		#sym_exe pdftotext functions_list
		sym_exe pdfinfo functions_list
		#sym_exe pdftopng  functions_list
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
