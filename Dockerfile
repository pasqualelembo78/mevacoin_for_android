# this docker file is intended for production usage to setup a node for the
# mevacoin blockchain. it is build using github actions.

# step 1: build the binary
FROM ubuntu:22.04 as builder

COPY . /usr/src/mevacoin
WORKDIR /usr/src/mevacoin

# install build dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libssl-dev \
    libffi-dev \
    python3-dev \
    gcc-11 \
    g++-11 \
    git \
    cmake \
    librocksdb-dev \
    libboost-all-dev \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-thread1.74.0 \
    libboost-date-time1.74.0 \
    libboost-chrono1.74.0 \
    libboost-regex1.74.0 \
    libboost-serialization1.74.0 \
    libboost-program-options1.74.0 \
    libicu70

# create the build directory
RUN mkdir build
WORKDIR /usr/src/mevacoin/build

# build and install
RUN cmake -DCMAKE_CXX_FLAGS="-g0 -Os -fPIC -std=gnu++17" .. && make

# step 2: create the final image
FROM ubuntu:22.04

WORKDIR /usr/src/mevacoin

COPY --from=builder /usr/src/mevacoin/start.sh .
COPY --from=builder /usr/src/mevacoin/build/src/mevacoind .
COPY --from=builder /usr/src/mevacoin/build/src/xkrwallet .
COPY --from=builder /usr/src/mevacoin/build/src/miner .
COPY --from=builder /usr/src/mevacoin/build/src/mevacoin-service .
COPY --from=builder /usr/src/mevacoin/build/src/wallet-api .

# set executable permissions
RUN chmod +x start.sh
RUN chmod +x mevacoind
RUN chmod +x xkrwallet
RUN chmod +x miner
RUN chmod +x mevacoin-service
RUN chmod +x wallet-api

EXPOSE 5000
EXPOSE 8070
EXPOSE 11897
EXPOSE 11898

# --data-dir is binded to a volume - this volume is binded when starting the container
# to start the container follow instructions on readme or in article published by marcus cvjeticanin on https://mjovanc.com
CMD [ "/bin/sh", "-c", "./mevacoind --enable-cors=* --rpc-bind-ip=127.0.0.1 --rpc-bind-port=11898" ]
