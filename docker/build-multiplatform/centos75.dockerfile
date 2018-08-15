FROM centos:7.5.1804 as build-base

ARG http_proxy
ARG https_proxy

SHELL ["/bin/bash", "-c"]

RUN yum -y update && yum install -y \
    automake \
    gcc \
    gcc-c++ \
    git \
    libtool \
    make \
    ncurses-devel \
    patch \
    unzip \
    vim 

RUN yum clean all

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

# Optionally install clang
ARG CLANG_VERSION
RUN if [ ! -z "$CLANG_VERSION" ]; then \
      spack/bin/spack -k install llvm@${CLANG_VERSION}+clang~gold~libcxx~lld~lldb~all_targets && \
      spack/bin/spack clean -a; \
    fi

###############################################################################
FROM scratch
COPY --from=build-base / /

# Configure bash to automatically build deps when the shell is created
RUN echo "source /home/build/spack/share/spack/setup-env.sh" >> ~/.bashrc
RUN echo "spack load environment-modules" >> ~/.bashrc
RUN echo "spack load otf2" >> ~/.bashrc

WORKDIR /home/build
