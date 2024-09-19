#!/bin/bash

CONTAINER_NAME="${USER}_cppfs"

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR

docker run -it --entrypoint /bin/bash --name "$CONTAINER_NAME" -v "$(pwd)/../:/cppfs"\
    -p 4443:4443 -p 3000:3000 -p 8080:8080 cppfs
