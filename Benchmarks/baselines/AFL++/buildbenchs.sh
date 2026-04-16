export ROOT=`pwd`
Action=$1


for dir in $(find . -maxdepth 1 -type d); do
    if [ "$dir" == "." ]; then
        continue
    fi

	cd $dir
    ./build.sh $Action
	cd -
done
