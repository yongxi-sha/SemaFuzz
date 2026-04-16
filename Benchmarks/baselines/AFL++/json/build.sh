

export ROOT=`pwd`
export benchName=$(basename "$ROOT")
export benchDir=`cd ../../../ && pwd`
export target=jsonfuzz
Action=$1

function afl_compile ()
{
	export JSON_PATH=$benchDir/$benchName/json/include

	export CC="afl-cc -fPIC -lshmQueue"
	export CXX="afl-c++ -fPIC -lshmQueue"
	export FUNCOVERAGE_OFF="TRUE"

	cp $benchDir/$benchName/$target ./ -rf
	cd $target
	make clean && make
	cd -
}

function compile ()
{
	export PKG_CONFIG_PATH=/root/anaconda3/lib/pkgconfig:$PKG_CONFIG_PATH

	if [ ! -d "$benchDir/$benchName/json" ]; then
		cd $benchDir/$benchName
		git clone https://github.com/nlohmann/json.git
		cd -
	fi

	# alf-cc comliling: for fuzzing	
	afl_compile

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
