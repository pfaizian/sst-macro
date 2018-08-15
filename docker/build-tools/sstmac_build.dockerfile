FROM ubuntu:18.04 as builder

ENV DEBIAN_FRONTEND=noninteractive
ARG http_proxy
ARG https_proxy
ARG CLANG_URL=http://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

SHELL ["/bin/bash", "-c"]

# Install dependencies
RUN apt-get update -y && apt-get install -y \
    aptitude \
    autoconf\
    automake \
    curl \
    g++ \
    git \
    libncurses-dev \
    libtool \
    libz-dev \
    make \
    python \
    unzip \
    vim

# Create a non-root user. Some build systems wrapped by spack (like tar) will
# complain and throw errors otherwise.
RUN useradd -m build
USER build
WORKDIR /home/build

# Install clang
RUN mkdir -p clang && \
    curl -k ${CLANG_URL} | tar xJ -C clang --strip-components=1

# Install otf2 dependencies via spack
RUN git clone https://github.com/spack/spack.git
RUN source spack/share/spack/setup-env.sh && \
    spack/bin/spack -k install otf2 environment-modules

# Clone and bootstrap SST/Macro
RUN git clone -b  ubuntu-buildsystem-fix  https://github.com/sstsimulator/sst-macro.git &&  \
    cd sst-macro && \
    ./bootstrap.sh

# do not use -static, does not pass build tests
# Build SST macro
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    cd sst-macro/ && \
    mkdir build && cd build/ && \
    CFLAGS='-fdata-sections -ffunction-sections -Wl,-gc-sections' CXXFLAGS='-fdata-sections -ffunction-sections -Wl,-gc-sections' CC=gcc CXX=g++ ../configure --with-clang=/home/build/clang && \
    make -j

# Check the build
# Run serially because it may otherwise throw timeout errors
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    cd sst-macro/build && \
    make check

# Install
USER root
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    spack load otf2 && \
    make -C sst-macro/build install
