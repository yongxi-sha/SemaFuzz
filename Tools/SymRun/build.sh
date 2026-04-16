#!/bin/bash

SEMSLICE=`pwd`
TOOL="SymRun"

function compileSVF ()
{
    SVF_BUILD=$SVF_DIR/Release-build
    if [ ! -d "$SVF_BUILD" ]; then
        cd $SVF_DIR  && source ./build.sh && cd -
    else
        cd $SVF_BUILD
        make -j4
    fi

    TARGET_DIR="$SVF_BUILD/svf-llvm/tools/$TOOL"
    if [ -f "$TARGET_DIR/$1" ]; then
        mv $TARGET_DIR/$1 $SVF_BUILD/$2/
    fi
}

SVF_TOOL=$SVF_DIR/svf-llvm/tools
if [ ! -L "$SVF_TOOL/$TOOL" ]; then
    cd $SVF_TOOL && ln -s $SEMSLICE "$TOOL" && cd -
fi

if ! grep -q "add_subdirectory($TOOL)" $SVF_TOOL/CMakeLists.txt; then
    mv $SVF_TOOL/CMakeLists.txt $SVF_TOOL/CMakeLists.txt-back
    echo "add_subdirectory($TOOL)" > $SVF_TOOL/CMakeLists.txt
    cat $SVF_TOOL/CMakeLists.txt-back >> $SVF_TOOL/CMakeLists.txt
    rm $SVF_TOOL/CMakeLists.txt-back
fi

# build semslice into executable
cd $SEMSLICE
compileSVF symrun bin
if [ -f "$SVF_BUILD/svf-llvm/tools/$TOOL/symrun" ]; then
    mv $SVF_BUILD/svf-llvm/tools/$TOOL/symrun $SVF_BUILD/bin/
fi



