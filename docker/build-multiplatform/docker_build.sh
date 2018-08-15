#!/bin/bash

TAG_PREFIX=${TAG_PREFIX:-}

# usage:
# mk_image <file> <tag> [ARGS]
function mk_image () {
  if [ $# -lt 2 ] ; then
    echo "ERROR: less than two arguments passed into mk_image()"
    return
  fi

  echo 
  echo "BUILDING IMAGE: $2"
  CMD="time docker build -t ${TAG_PREFIX}${2} --build-arg http_proxy=$HTTP_PROXY --build-arg https_proxy=$HTTPS_PROXY -f $1"
  shift 2
  CMD="$CMD $@ ."
  echo "$CMD"
  $CMD
}

mk_image ubuntu1804.dockerfile "ubuntu18.04:NO_LLVM"       
mk_image ubuntu1804.dockerfile "ubuntu18.04:LLVM4" '--build-arg CLANG_URL=http://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz'
mk_image ubuntu1804.dockerfile "ubuntu18.04:LLVM5" '--build-arg CLANG_URL=http://releases.llvm.org/5.0.2/clang+llvm-5.0.2-x86_64-linux-gnu-ubuntu-16.04.tar.xz'
mk_image ubuntu1804.dockerfile "ubuntu18.04:LLVM6" '--build-arg CLANG_URL=http://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz'

mk_image centos75.dockerfile "centos7.5:NO_LLVM"
mk_image centos75.dockerfile "centos7.5:LLVM4" '--build-arg CLANG_VERSION=4.0.1'
mk_image centos75.dockerfile "centos7.5:LLVM5" '--build-arg CLANG_VERSION=5.0.1'
mk_image centos75.dockerfile "centos7.5:LLVM6" '--build-arg CLANG_VERSION=6.0.1'
