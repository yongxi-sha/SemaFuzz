

export ROOT=`pwd`
export target=mp4v2-2.1.3
Action=$1

function wllvm_compile ()
{
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	export CXX="wllvm++ -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w"
	
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	cmake ../$target
	make -j4
		
	extract-bc mp4art #&& extract-bc mp4extract && extract-bc mp4file && extract-bc mp4subtitle && extract-bc mp4track
	cp *.bc $ROOT/
	cd $ROOT/
	
	export ONLY_CG="true"
	wpa -fspta -dump-callgraph mp4art.bc      #&& mv callgraph_final.dot mp4art_callgraph_final.dot
	#wpa -fspta -dump-callgraph mp4extract.bc  && mv callgraph_final.dot mp4extract_callgraph_final.dot
	#wpa -fspta -dump-callgraph mp4file.bc     && mv callgraph_final.dot mp4file_callgraph_final.dot
	#wpa -fspta -dump-callgraph mp4subtitle.bc && mv callgraph_final.dot mp4subtitle_callgraph_final.dot
	#wpa -fspta -dump-callgraph mp4track.bc    && mv callgraph_final.dot mp4track_callgraph_final.dot
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
}


function compile ()
{
	if [ ! -d "$ROOT/$target" ]; then
		tar -xvf $target.tar.gz
	fi
	
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	if [ "$Action" == "all" ]; then
		# wllvm compiling: for SVF analysis
		wllvm_compile
		
		# alf-cc comliling: for fuzzing
		afl_compile

		# function summarization
		sym_exe mp4art      functions_list
		#sym_exe mp4extract  functions_list
		#sym_exe mp4file     functions_list
		#sym_exe mp4subtitle functions_list
		#sym_exe mp4track    functions_list
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
