The cppfs is expected to be built inside docker container.

Firstly, build docker image:
```bash
cd docker
docker build -t cppfs .
```

Then run docker container:
```bash
cd docker
./run.sh
```
Note that `./run.sh` exposes only 4443 port, so it won't be possible to connect to any other port from outside the container.
