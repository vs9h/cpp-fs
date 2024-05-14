FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-dev openssl make cmake && \
    apt-get clean

RUN pip3 install conan==2.3.0 --break-system-packages

WORKDIR /cppfs/

EXPOSE 4443

CMD ["bash"]
