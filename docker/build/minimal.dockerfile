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
RUN git clone -b devel https://github.com/sstsimulator/sst-macro.git &&  \
    cd sst-macro && \
    ./bootstrap.sh

# Build SST macro
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    spack load llvm otf2 && \
    cd sst-macro/ && \
    mkdir build && cd build/ && \
    CFLAGS='-fdata-sections -ffunction-sections -static' CXXFLAGS='-fdata-sections -ffunction-sections -static' CC=gcc CXX=g++ ../configure --with-clang=/home/build/clang && \
    make -j

# Install
USER root
RUN source `spack/bin/spack location -i environment-modules`/Modules/init/bash && \
    source spack/share/spack/setup-env.sh && \
    spack load otf2 llvm && \
    make -C sst-macro/build install

RUN echo "NOTE: The executables have dynamic links that must be copied to the final image:" && \
    ldd /usr/local/bin/sstmac /usr/local/bin/sstmac_clang && \
    libs=`ldd /usr/local/bin/{sstmac,sstmac_clang} | awk '{print $3}' | sed '/^$/d' | sort -u` && \
    mkdir libs && \
    cp $libs $(readlink /lib64/ld-linux-x86-64*) libs

# Flatten the symlink in /lib64 so it can be copied directly into the squashed image
RUN cp --remove-destination `readlink /lib64/ld-linux-x86-64.so.2` /lib64/ld-linux-x86-64.so.2

## Wipe out the build image
## Save the binaries and the components necessary for dynamic linking
FROM scratch
COPY --from=builder /lib64 /lib64
COPY --from=builder /usr/local/bin/sstmac /usr/local/bin/sstmac_clang /
COPY --from=builder /home/build/libs /lib

# Just to show it runs
CMD ["/sstmac", "-h"]
