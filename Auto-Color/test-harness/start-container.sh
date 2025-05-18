#!/bin/bash
set -euo pipefail

# Check if current user has permission to access Docker socket
# If not, use sudo to run Docker commands
docker ()
{
    if [ -w ${DOCKER_SOCK_PATH:-/var/run/docker.sock} ]; then
        docker "$@"
    else
        sudo docker "$@"
    fi
}

TEST_PATH="$1"
BINARY_PATH="$(dirname "$TEST_PATH")/../binary"

# Start a Debian Docker container
CONTAINER_ID=$(docker run -d --rm -it debian:stable-slim sh)

# Copy the provided executable to the container
docker cp "$TEST_PATH" "$CONTAINER_ID":/test 1>/dev/null
docker cp "$BINARY_PATH" "$CONTAINER_ID":/binary 1>/dev/null

# Set permissions on the files inside the container
docker exec "$CONTAINER_ID" chmod 777 / /test /binary

# Execute the provided file inside the container
docker exec "$CONTAINER_ID" bash -c "TERM=xterm-256color /test --color always ${@:2}"

echo "container_id=$CONTAINER_ID"

# Remove the container
# docker rm --force "$CONTAINER_ID" 1>/dev/null