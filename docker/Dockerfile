FROM llvm7-ubuntu:jammy

LABEL maintainer="dmitry@kernelgen.org"

ENV DEBIAN_FRONTEND noninteractive
ENV LC_ALL C.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

RUN apt-get update && \
        apt-get -y --no-install-recommends install \
                cmake \
                git \
                ninja-build \
                g++-11 && \
        apt-get clean

COPY . /root/

WORKDIR /root
