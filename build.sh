
BASE_DIR=`pwd`

Action=$1

if [ "$Action" == "clean" ]; then
	cd $BASE_DIR/AFLplusplus && make clean && cd -
	cd $BASE_DIR/Common && make clean && cd -

	cd $BASE_DIR/SVF
	if [ -d "Release-build" ]; then rm -rf Release-build z3.obj; fi
	cd -

	cd $BASE_DIR/Tools && ./buildtools.sh clean && cd -
	exit 0
fi

BASE_DIR=`pwd`
$BASE_DIR/Tools/delshm.sh
cp $BASE_DIR/Tools/delshm.sh /usr/bin/

# 1. dependences in common
cd $BASE_DIR/Common && make clean && make
if [ ! -f "/usr/include/json.hpp" ]; then
	cp $BASE_DIR/Tools/json.hpp /usr/include/ -f
fi


# 2. build AFL++
cd $BASE_DIR/AFLplusplus
if [ ! -f "afl-cc" ]; then
    make source-only
else
    make
fi


# 3. build SVF
cd $BASE_DIR/SVF
if [ -d "Release-build" ]; then
    cd Release-build
	make
else
	source ./build.sh
fi
cd -


# 4. build toolkit
cd $BASE_DIR/Tools && ./buildtools.sh && cd -