
bench=`cd ../../Benchmarks/arrow && pwd`
target=arrow-apache-arrow-19.0.1
function compile_arrow ()
{
	cd $bench
	tar -zxvf $target.tar.gz
	
	cd $target/cpp
	if [ -d "build" ]; then rm -rf build; fi
	mkdir build && cd build
	
	cmake .. -GNinja \
    	-DCMAKE_BUILD_TYPE=Debug \
    	-DCMAKE_INSTALL_PREFIX=/usr/local \
    	-DARROW_JSON=ON \
    	-DARROW_CSV=ON \
    	-DARROW_IPC=ON \
    	-DARROW_IO=ON
	cmake --build . -- -j1
	unset FUNCTIONS_LIST

	export ARROW_PATH="$ROOT/$target/cpp/src"
	export ARROW_BUILD="$ROOT/$target/cpp/build/src"
	export ARROW_LIB="$ROOT/$target/cpp/build/debug"
	cd $bench/arrowfuzz
	make clean && make
	cd -
	
	
}

#install arrow first
compile_arrow

export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/libstdc++.so.6:$bench/$target/cpp/build/debug:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
cp $bench/arrowfuzz/arrowfuzz ./
./arrowfuzz null_deference 