#!/usr/bin/env bash

Action="${1:-help}"
Function="${2:-}"
NInstance="${3:-1}"
LLM="${4:-}"
Image="${5:-daybreak2019/semafuzz:v1.0}"
GitPush="no"
ContainNet="ollama-network"

ProjName="SemaFuzz"
ResultName="${ProjName}_result"

DefaultLLM="ds-7b"

function Help() {
  cat <<EOF

Usage:
  ${0} run [experiment] [docker_image]
    e.g. ${0} run nollm daybreak2019/semfuzz:v1.3

  ${0} collect [experiment] [GitPush=yes/no]
    e.g. ${0} collect nollm no

EOF
}

function Collect() {
  local ExpName="$1"
  local llm="$2"
  local FuzzName="${ProjName}-${llm}-${ExpName}"
  local resultDir="$ResultName"

  echo "### Collecting experiment results from container: $FuzzName"

  # Optionally check if container is running or exist
  docker ps -a --format '{{.Names}}' | grep -w "$FuzzName" &>/dev/null || {
    echo "Warning: Container $FuzzName not found or not running."
    # Possibly exit or continue
  }

  # Execute the 'collect' inside container
  docker exec -it -w "/root/${ProjName}/Experiments" "$FuzzName" \
    bash autorun.sh collect "$ExpName"

  # Decide local copy directory
  local LocalDir="${resultDir}/ablation/${ExpName}/${llm}"
  if [ "$ExpName" == "all" ]; then
    LocalDir="${resultDir}/${ProjName}/${llm}"
  fi
  mkdir -p "$LocalDir"

  # Copy results from container to host
  echo "### Copying results to $LocalDir"
  docker cp "${FuzzName}:/root/${ProjName}/Experiments/${resultDir}/." "$LocalDir"

  # Git push if required
  if [ "$GitPush" == "yes" ]; then
    RepoDir="git_${resultDir}_$llm"
    git clone git@github.com:sneakyggg/Baseline.git "$RepoDir"
    cd "$RepoDir" || exit 1
    cp -r "../${LocalDir}/." .
    git add .
    git commit -m "${ExpName}-$(date +%Y-%m-%d_%H:%M:%S)"
    git push
    cd ..
    rm -rf "$RepoDir"
  fi
}

function RunOne() {
  local ExpName="$1"
  local llm="$2"
  local FuzzName="${ProjName}-${llm}-${ExpName}"
  local ContainerPara="--security-opt seccomp=unconfined --privileged --network $ContainNet"

  echo "### Running fuzzers for $ExpName in container: $FuzzName..."

  # Remove old container if exists
  docker ps -a --format '{{.Names}}' | grep -w "$FuzzName" &>/dev/null && {
    echo "Container '$FuzzName' already exists, removing it..."
    docker rm -f "$FuzzName"
  }

  # Start container
  docker run -itd $ContainerPara --name "$FuzzName" "$Image"

  # Kick off fuzzing
  docker exec -d -w "/root/${ProjName}/Experiments" "$FuzzName" \
    bash autorun.sh run "$ExpName" "$llm" "$NInstance"
}


case "$Action" in
    help)
      Help
      ;;

    run)
      if [[ -z "$Function" ]]; then
        echo "Please specify the experiment name (e.g. 'nollm', 'nopath', 'diffllm')"
        exit 1
      fi

      case "$Function" in
        "all"|"nollm"|"nopath"|"randompath"|"diffllm")
            RunOne "$Function" "$LLM"
          ;;
        *)
          echo "Unsupported experiment name: $Function"
          echo "Supported experiments: all, nollm, nopath, randompath, diffllm"
          exit 1
          ;;
      esac
      ;;

    collect)
      # Re-map the parameters to match usage
      ExpToCollect="$2"
      LLm="$3"
      GitPush="${4:-yes}"

      if [[ -z "$ExpToCollect" ]]; then
        echo "Please specify which experiment to collect [all|nollm|nopath|randompath|diffllm]"
        exit 1
      fi

      if [ "$ExpToCollect" == "diffllm" ]; then
        Collect "$ExpToCollect" "ds-1.5b"
        Collect "$ExpToCollect" "ds-7b"
        Collect "$ExpToCollect" "ds-14b"
        wait
      else
        Collect "$ExpToCollect" "$LLm"
      fi
      ;;

    *)
      echo "### Unsupported action: $Action"
      Help
      exit 1
      ;;
esac
