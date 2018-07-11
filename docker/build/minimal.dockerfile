FROM ubuntu:18.04 as builder

ENV DEBIAN_FRONTEND=noninteractive
ARG http_proxy
ARG https_proxy

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

# Create a non-root user. Some build systems in by spack (tar) will
# complain and throw errors when compiled as root.
RUN useradd -m build
USER build
WORKDIR /home/build

# Install clang and otf2 dependencies via spack
RUN git clone https://github.com/spack/spack.git

RUN source spack/share/spack/setup-env.sh && \
    spack/bin/spack -k install otf2 environment-modules llvm+clang

# Clone and bootstrap SST/Macro
RUN git clone -b devel https://github.com/sstsimulator/sst-macro.git &&  \
    cd sst-macro && \
    ./bootstrap.sh

# Build SST macro
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    spack load llvm otf2 && \
    cd sst-macro/ && \
    mkdir build && cd build/ && \
    CFLAGS='-fdata-sections -ffunction-sections -static' CXXFLAGS='-fdata-sections -ffunction-sections -static' CC=gcc CXX=g++ ../configure --with-clang=$(dirname $(dirname $(which clang))) && \
    make -j

# Install
USER root
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    spack load otf2 llvm && \
    make -C sst-macro/build install

# Wipe out the build image
# Save the binaries and the components necessary for dynamic linking
FROM scratch
COPY --from=ubuntu:18.04 /usr/lib /usr/lib
COPY --from=ubuntu:18.04 /lib64 /lib64
COPY --from=builder /lib/x86_64-linux-gnu /lib/x86_64-linux-gnu
COPY --from=builder /usr/local/bin/sstmac /usr/local/bin/sstmac_clang /

# Just to show it runs
CMD ["/sstmac", "-h"]
