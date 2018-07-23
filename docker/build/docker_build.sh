#!/bin/bash

# Compile sst macro
docker build -t sstmac_build -f sstmac_build.dockerfile .

# Containerize 'sstmac' program
docker build -t sstmac -f sstmac.dockerfile .

# Containerize sstmac compilation tools
docker build -t sstcxx -f sstcxx.dockerfile .
