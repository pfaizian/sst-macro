FROM sstmac_build as builder

# Flatten the symlink in /lib64 so it can be copied directly into squashed images
RUN cp --remove-destination `readlink /lib64/ld-linux-x86-64.so.2` /lib64/ld-linux-x86-64.so.2

# Put library deps into a folder so they can be copied into the final image
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
RUN echo "NOTE: 'sstmac' has dynamic links that must be copied to the final image:" && \
    ldd /usr/local/bin/sstmac && \
    libs=`ldd /usr/local/bin/sstmac | awk '{print $3}' | sed '/^$/d' | sort -u` && \
    mkdir libs && \
    cp $libs libs

FROM scratch
COPY --from=builder /lib64 /lib64
COPY --from=builder /usr/local/bin/sstmac /
COPY --from=builder /home/build/libs /lib

# Just to show it runs
ENTRYPOINT ["/sstmac"]
CMD ["-h"]
