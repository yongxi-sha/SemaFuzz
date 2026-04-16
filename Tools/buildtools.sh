
export BASE_PATH=`pwd`
export DOTSHOW=$BASE_PATH/DotShow
export SEMLEARN=$BASE_PATH/SemLearn
#export SEMSLICE=$BASE_PATH/SemSlice
export SYMRUN=$BASE_PATH/SymRun

Action=$1

function remove ()
{
    FILE=$1
    if [ -d "$FILE" ]; then rm -rf $FILE; fi
}

if [ "$Action" == "clean" ]; then

    cd $SEMLEARN/semgdump 
    remove build
    make clean

    cd $SEMLEARN/semcore
    remove build
    make clean

    cd $SEMLEARN/
    remove build
    remove semlearn.egg-info
    remove dist

    exit 0
fi


#1. build symrun
cd $SYMRUN
./build.sh

#2. build dotshow
cd $DOTSHOW
remove build
python setup.py install


#3. build semlearn
#3.1 semcore
cd $SEMLEARN/semcore
remove build
python setup.py install
make clean && make

#3.2 semgdump
cd $SEMLEARN/semgdump
remove build
python setup.py install
make clean && make

cd $SEMLEARN
remove build
python setup.py install

#3.3 other tools
cd $BASE_PATH/PLog
python setup.py install
 
 #4. build llm proxy
 cd $BASE_PATH/LLMProxy
 python setup.py install