#!/bin/bash

# Compile sst macro
docker build -t sstmac_build -f sstmac_build.dockerfile .

# Containerize 'sstmac' program
docker build -t sstmac -f sstmac.dockerfile .

# Containerize 'sstmac_clang' program
docker build -t sstmac_clang -f sstmac_clang.dockerfile .
