FROM sstmac_build as builder

ENV DEBIAN_FRONTEND=noninteractive
ARG http_proxy
ARG https_proxy

# Flatten the symlink in /lib64 so it can be copied directly into squashed images
RUN cp --remove-destination `readlink /lib64/ld-linux-x86-64.so.2` /lib64/ld-linux-x86-64.so.2

# Put library deps into a folder so they can be copied into the final image
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
RUN echo "NOTE: 'sstmac_clang' has dynamic links that must be copied to the final image:" && \
    ldd /usr/local/bin/sstmac_clang && \
    libs=`ldd /usr/local/bin/sstmac_clang | awk '{print $3}' | sed '/^$/d' | sort -u` && \
    mkdir libs && \
    cp $libs libs

FROM ubuntu:18.04
COPY --from=builder /lib64 /lib64
COPY --from=builder /usr/local/bin/ /bin
COPY --from=builder /home/build/libs /lib

RUN apt-get update -y && apt-get upgrade -y && apt-get install -y \
    python \
    g++ && \
    rm -rf /var/lib/apt/lists/*
# Just to show it runs
#ENTRYPOINT ["/sstmac_clang"]
#CMD ["-help"]
