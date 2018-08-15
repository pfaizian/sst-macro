# Installs dependencies for ubuntu
FROM ubuntu:18.04 as build-base

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

# Clean up apt
RUN apt-get -y autoremove && apt-get -y clean

# Create a non-root user. Some build systems wrapped by spack (like tar) will
# complain and throw errors when built as root
RUN useradd -m build

RUN cp /root/.bashrc /home/build/.bashrc
RUN chown build: /home/build/.bashrc

USER build
WORKDIR /home/build

# Install otf2 dependencies via spack
RUN git clone https://github.com/spack/spack.git
RUN source spack/share/spack/setup-env.sh && \
    spack/bin/spack -k install otf2 environment-modules && \
    spack/bin/spack clean -a

# Remove some of the inordinate amount of space taken by documentation
RUN rm -rf spack/opt/spack/*/*/*/share

RUN git clone https://github.com/sstsimulator/sst-macro.git

###############################################################################
# separate build stages for caching
FROM alpine:latest as fetch-clang

ARG http_proxy
ARG https_proxy

# install curl
RUN apk update && apk add curl

# Install clang when CLANG_URL is defined
ARG CLANG_URL
RUN mkdir -p /clang && \
    if [ ! -z "$CLANG_URL" ]; then curl -k ${CLANG_URL} | tar xJ -C clang --strip-components=1; fi;

###############################################################################
FROM scratch
COPY --from=build-base / /
COPY --from=fetch-clang /clang/ /clang/

# Configure bash to automatically load spack environment
RUN echo "source spack/share/spack/setup-env.sh" >> ~/.bashrc
RUN echo "spack load environment-modules" >> ~/.bashrc
RUN echo "spack load otf2" >> ~/.bashrc

WORKDIR /home/build
