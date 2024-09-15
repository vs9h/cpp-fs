#!/bin/bash

CONTAINER_NAME="${USER}_cppfs"

docker run -it --entrypoint /bin/bash --name "$CONTAINER_NAME" -v "$(pwd)/../:/cppfs" -p 4443:4443 cppfs
