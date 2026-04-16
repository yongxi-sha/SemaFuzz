#  !bash
Version=14.0.6

# 1. extract source code
if [ ! -d "llvm-$Version.src" ]; then
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/llvm-$Version.src.tar.xz
	tar -xvf llvm-$Version.src.tar.xz
	
	wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/clang-$Version.src.tar.xz
	tar -xvf clang-$Version.src.tar.xz
	mv clang-$Version.src llvm-$Version.src/tools/
	
	wget https://github.com/llvm/llvm-project/releases/download/llvmorg-$Version/compiler-rt-$Version.src.tar.xz
	tar -xvf compiler-rt-$Version.src.tar.xz
	mv compiler-rt-$Version.src llvm-$Version.src/projects/
	
	apt install autoconf bison flex m4 libgmp-dev libmpfr-dev libssl-dev
	cp CMakeLists.txt llvm-$Version.src/
fi


# 2. build binutils-2.32
if [ ! -f "binutils-2.32.tar.gz" ]; then
	wget https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.gz
	tar -xzf binutils-2.32.tar.gz
fi
if [ ! -d "binu_build" ]; then mkdir binu_build; fi
cd binu_build
../binutils-2.32/configure --enable-gold --enable-plugins --disable-werro
make all-gold
cd -

# 3. build llvm with gold plugin
if [ ! -d "build" ]; then mkdir build; fi
cd build
cmake -DCMAKE_BUILD_TYPE:String=Release -DLLVM_BINUTILS_INCDIR=../binutils-2.32/include ../llvm-$Version.src
make -j4
cd -
