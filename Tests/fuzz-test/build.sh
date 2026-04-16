
Action=$1

if [ "$Action" == "clean" ]; then
	for item in test*
	do
		cd $item 
		make clean
		rm -f demo.*
		cd -
	done
	
	exit 0
fi

for item in test*
do
	export CC="wllvm -O0 -Xclang -disable-O0-optnone -g -save-temps=obj -fno-discard-value-names -w" 
	cd $item && make clean && make
	extract-bc demo

	wpa -ander -dump-callgraph demo.bc
	dot -Tpng callgraph_final.dot -o callgraph_final.png

	wpa -type -dump-icfg demo.bc
	dot -Tpng icfg_initial.dot -o icfg_initial.png
	dot -Tpng vfg_initial.dot -o vfg_initial.png
	cd -

	export CC="afl-cc"
	cd $item && make clean && make
	cd -
done