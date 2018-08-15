#!/bin/bash

# Compile sst macro
time docker build -t sstmac_build -f sstmac_build.dockerfile .

# Containerize 'sstmac' program
time docker build -t sstmac -f sstmac.dockerfile .

# Containerize sstmac compilation tools
time docker build -t sstcxx -f sstcxx.dockerfile .
