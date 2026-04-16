export ROOT=`pwd`
Action=$1


for dir in $(find . -maxdepth 1 -type d); do
    if [[ "$dir" == "." || "$dir" == "./baselines" ]]; then
        continue
    fi
    
    delshm.sh
	cd $dir
    ./build.sh $Action
	cd -
done
