FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    git \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libprotobuf-dev \
    libgrpc++-dev \
    libgrpc-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root

ENV GRPC_RELEASE_TAG v1.76.0
ENV GRPC_INSTALL_DIR /root/.local
RUN git clone  https://github.com/grpc/grpc -b ${GRPC_RELEASE_TAG} --recurse-submodules --depth 1 --shallow-submodules
RUN mkdir -p /root/grpc/cmake/build
WORKDIR /root/grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON \
        -DgRPC_BUILD_TESTS=OFF \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_INSTALL_PREFIX=$GRPC_INSTALL_DIR \
        ../..
RUN make -j1
RUN make install 

WORKDIR /root/app

COPY . .

RUN protoc \
    --grpc_out=. \
    --cpp_out=. \
    --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
    rpc_sync.proto

RUN cmake -B build -S . && \
    cmake --build build -j"$(nproc)"

CMD ["echo", "specify CMD"]
