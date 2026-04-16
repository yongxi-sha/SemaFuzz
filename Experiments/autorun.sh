#!/usr/bin/env bash

ProjName="SemaFuzz"

# Environment variables
export CONDA_SHLVL=1
export LD_LIBRARY_PATH="/root/$ProjName/SVF/Release-build/svf:/root/$ProjName/SVF/Release-build/svf-llvm:/usr/lib:/usr/lib64:/root/$ProjName/SVF/z3.obj/bin:/root/anaconda3/lib:"
export CONDA_EXE=/root/anaconda3/bin/conda
export CONDA_PREFIX=/root/anaconda3
export DYLD_LIBRARY_PATH="/root/$ProjName/SVF/z3.obj/bin:"
export SVF_DIR="/root/$ProjName/SVF"
export _CE_M=
export LLVM_COMPILER=clang
export Z3_DIR="/root/$ProjName/SVF/z3.obj"
export HOME=/root
export CONDA_PYTHON_EXE=/root/anaconda3/bin/python
export CONDA_PROMPT_MODIFIER="(base) "
export SEMFUZZ_DIR="/root/$ProjName"
export LLVM_PATH=/root/tools
export PATH="/root/$ProjName/SVF/Release-build/bin:/root/$ProjName/SVF/z3.obj/bin:/root/anaconda3/bin:/root/anaconda3/condabin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/root/tools/build/bin:/root/$ProjName/AFLplusplus"
export CLANG_PATH=/root/tools/build/bin
export CONDA_DEFAULT_ENV=base
export LLVM_DIR=/root/tools/build

ACTION="$1"
ExpName="$2"
LLM="$3"
NProc="$4"

# Points to ../Benchmarks relative to the location of this script
BenchmarkDir="$(cd ../Benchmarks && pwd)"

function Help() {
  echo "Usage: $0 <action> <experiment>"
  echo "Actions:"
  echo "  run       - runs fuzzing with a given experiment name"
  echo "  collect   - collects results (implementation needed)"
  echo
  echo "Experiments can be: all, nollm, nopath, randompath"
  echo
  exit 1
}

# Copy function that checks if the source file exists
function copyFile() {
  local srcFile="$1"
  local dstDir="$2"

  mkdir -p "$dstDir"

  if [ -f "$srcFile" ]; then
    cp "$srcFile" "$dstDir"
  fi
}

# If no action provided, show help
[ -z "$ACTION" ] && Help

case "$ACTION" in
  run)
    # We require an experiment name
    if [ -z "$ExpName" ]; then
      echo "Error: Please specify an experiment (all|nollm|nopath|randompath|diffllm)"
      exit 1
    fi

    # Move into the benchmark directory and run the appropriate script
    cd "$BenchmarkDir" || exit 1

    case "$ExpName" in
      all)
        ./runAllFuzz.sh $LLM $NProc -t
        ;;
      nollm)
        ./runAllFuzz.sh ds-7b $NProc -t -l
        ;;
      nopath)
        ./runAllFuzz.sh ds-7b $NProc -t -x
        ;;
      randompath)
        ./runAllFuzz.sh ds-7b $NProc -t -r
        ;;
      diffllm)
        ./runAllFuzz.sh $LLM $NProc -t -r
        ;;
      *)
        echo "Unsupported experiment name: $ExpName"
        echo "Supported: all, nollm, nopath, randompath"
        exit 1
        ;;
    esac

    # Return to previous directory
    cd - >/dev/null 2>&1
    ;;

  collect)
    # If needed, you can parse a second argument for ExpName here.
    # e.g., ExpName="$2" (already set at the top).
    # Possibly handle blank or default behavior.

    ResultName="${ProjName}_result"

    # Clean up any existing results directory
    if [ -d "$ResultName" ]; then
      rm -rf "$ResultName"
    fi
    mkdir -p "$ResultName"

    echo "Collecting results for experiment: '$ExpName'..."
    echo "BenchmarkDir: $BenchmarkDir"
    echo "Results will be in: $ResultName/"

    # Loop over subdirectories in Benchmarks
    for benchPath in "$BenchmarkDir"/*; do
      # Skip if not a directory
      [ -d "$benchPath" ] || continue

      benchName="$(basename "$benchPath")"

      # Skip these specific directories or names if needed
      if [ "$benchName" == "baselines" ]; then
        continue
      fi

      # Copy desired files from each bench directory if they exist
      copyFile "$benchPath/func_cov.txt"         "$ResultName/$benchName"
      copyFile "$benchPath/path_explore_stat.log" "$ResultName/$benchName"
      copyFile "$benchPath/fuzz/perf_periodic.txt" "$ResultName/$benchName"
    done
    ;;

  *)
    echo "### Unsupported action: $ACTION"
    Help
    ;;
esac
