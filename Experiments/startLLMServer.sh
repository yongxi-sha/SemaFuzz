#!/usr/bin/env bash
#
# startLLMServer.sh
#
# Description:
#   1. Takes an optional argument for the host port (default: 5001).
#   2. Checks if a Docker container named 'ollama-server' exists:
#        - If it does, exit (no re-setup needed).
#   3. If not, checks if the image 'yhryyq/ollama-server:v1.0' is locally available
#      and pulls it if necessary.
#   4. Ensures a Docker network named 'ollama-network' exists.
#   5. Runs a new container named 'ollama-server' on that network,
#      mapping the host port to the container’s internal port 5001.
#
# Usage:
#   ./startLLMServer.sh [HOST_PORT]
#   e.g. ./startLLMServer.sh 5001
#
#   Access the container from another container using:
#       http://ollama-server:5001
#   And from the host via:
#       http://localhost:[HOST_PORT]
#

set -euo pipefail

###############################################################################
# CONFIG & INPUT
###############################################################################
CONTAINER_NAME="ollama-server"
IMAGE_NAME="yhryyq/ollama-server:v1.8"
NETWORK_NAME="ollama-network"

# Read the first argument as HOST_PORT, default to 5001 if not provided.
HOST_PORT="${1:-5001}"

# Set container’s internal port to 5001 for inter-container connectivity.
CONTAINER_PORT="5001"

###############################################################################
# FUNCTIONS
###############################################################################

install_nvidia_container_toolkit() {
    # Check if nvidia-container-toolkit is already installed
    if dpkg-query -W -f='${Status}' nvidia-container-toolkit 2>/dev/null | grep -q "install ok installed"; then
        echo "nvidia-container-toolkit is already installed."
    else
        echo "nvidia-container-toolkit is not installed. Installing now..."
        
        # Override the distribution to a supported version (e.g., Ubuntu 22.04)
        export distribution=ubuntu22.04

        # Add the NVIDIA GPG key
        curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -

        # Add the NVIDIA Docker repository for the specified distribution
        curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

        # Update the package list and install the toolkit
        sudo apt-get update
        sudo apt-get install -y nvidia-container-toolkit

        # Restart Docker to apply changes
        sudo systemctl restart docker

        echo "nvidia-container-toolkit installation complete."
    fi
}

# check_container_exists: returns 0 if container exists, 1 if not.
check_container_exists() {
  docker ps -a --format '{{.Names}}' | grep -Fxq "${CONTAINER_NAME}"
}

# check_image_exists: returns 0 if image exists locally, 1 if not.
check_image_exists() {
  local image_id
  image_id=$(docker images -q "${IMAGE_NAME}" 2>/dev/null || true)
  if [ -n "${image_id}" ]; then
    return 0  # image present
  else
    return 1  # image not found locally
  fi
}

# pull_image: pulls the image from the Docker registry.
pull_image() {
  echo "Pulling image '${IMAGE_NAME}'..."
  docker pull "${IMAGE_NAME}"
}

# create_network_if_needed: ensures the specified Docker network exists.
create_network_if_needed() {
  if ! docker network ls --format '{{.Name}}' | grep -wq "${NETWORK_NAME}"; then
    echo "Creating network '${NETWORK_NAME}'..."
    docker network create "${NETWORK_NAME}"
  else
    echo "Network '${NETWORK_NAME}' already exists."
  fi
}

# run_container: creates and starts the container in detached mode on the specified network.
run_container() {
  echo "Running container '${CONTAINER_NAME}' on host port ${HOST_PORT} (mapped to container port ${CONTAINER_PORT})..."
  docker run -d --gpus=all\
    --name "${CONTAINER_NAME}" \
    --network "${NETWORK_NAME}" \
    -p "${HOST_PORT}:${CONTAINER_PORT}" \
    "${IMAGE_NAME}"

  echo "Ollama server is now running in container '${CONTAINER_NAME}'."
  echo "Access it from other containers via: http://${CONTAINER_NAME}:${CONTAINER_PORT}"
  echo "Access it from the host via: http://localhost:${HOST_PORT}"
}

###############################################################################
# MAIN
###############################################################################

main() {
  install_nvidia_container_toolkit
  
  echo "=== 1. Checking if container '${CONTAINER_NAME}' already exists ==="
  if check_container_exists; then
    echo "Container '${CONTAINER_NAME}' already exists. Skipping setup."
    exit 0
  fi

  echo "=== 2. Container not found. Checking if the image '${IMAGE_NAME}' is available ==="
  if check_image_exists; then
    echo "Image '${IMAGE_NAME}' is already present locally."
  else
    pull_image
  fi

  echo "=== 3. Ensuring network '${NETWORK_NAME}' exists for container-to-container communication ==="
  create_network_if_needed

  echo "=== 4. Running the '${CONTAINER_NAME}' container ==="
  run_container

  echo "=== Done! ==="
}

# Run the main function
main
