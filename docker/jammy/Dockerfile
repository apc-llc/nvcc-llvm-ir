# syntax=docker/dockerfile:experimental
#===- llvm/utils/docker/jammy/build/Dockerfile -------------------------===//
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
# Stage 1. Check out LLVM source code and run the build.
FROM nvcr.io/nvidia/nvhpc:22.11-devel-cuda_multi-ubuntu22.04 as builder
LABEL maintainer "dmitry@kernelgen.org"
# Install build dependencies of llvm.
# First, Update the apt's source list and include the sources of the packages.
RUN grep deb /etc/apt/sources.list | \
    sed 's/^deb/deb-src /g' >> /etc/apt/sources.list
# Install compiler, python and subversion.
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates gnupg \
           build-essential cmake make python3 zlib1g wget subversion unzip git
# Install a newer ninja release. It seems the older version
# randomly crashes when compiling llvm.
RUN wget "https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip" && \
    echo "d2fea9ff33b3ef353161ed906f260d565ca55b8ca0568fa07b1d2cab90a84a07 ninja-linux.zip" \
        | sha256sum -c  && \
    unzip ninja-linux.zip -d /usr/local/bin && \
    rm ninja-linux.zip
# Install mold as recommended here: https://dev-docs.kicad.org/en/build/linux/
RUN git clone https://github.com/rui314/mold.git && \
    mkdir mold/build && \
    cd mold/build && \
    git checkout v1.7.1 && \
    ../install-build-deps.sh && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=c++ -G Ninja .. && \
    cmake --build . && \
    cmake --install .

ADD checksums /checksums
ADD scripts /scripts

# Checkout the source code.
# Run the build. Results of the build will be available at /tmp/llvm-install/.
ARG checkout_args
ARG buildscript_args
RUN --mount=type=tmpfs,destination=/tmp \
    /scripts/checkout.sh ${checkout_args} && \
    /scripts/build_install_llvm.sh --to /llvm-install ${buildscript_args}


# Stage 2. Produce a minimal release image with build results.
FROM nvcr.io/nvidia/nvhpc:22.11-devel-cuda_multi-ubuntu22.04
LABEL maintainer "dmitry@kernelgen.org"
# Install packages for minimal useful image.
RUN apt-get update && \
    apt-get install -y --no-install-recommends libstdc++-9-dev binutils && \
    rm -rf /var/lib/apt/lists/*
# Copy build results of stage 1 to /usr/local.
COPY --from=builder /llvm-install/ /usr/local/

