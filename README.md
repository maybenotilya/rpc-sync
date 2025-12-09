# RPC sync

2 processes syncronization via gRPC communication.

# How to run
1. Install `protoc` compiler and `grpc_cpp_plugin`. For example installation for Ubuntu:
```
sudo apt update && sudo apt upgrade
sudo apt install -y build-essential \
                    autoconf \
                    libtool \
                    pkg-config \ 
                    protobuf-compiler \ 
                    protobuf-compiler-grpc \
                    libprotobuf-dev \
                    libprotoc-dev \
                    libprotobuf-c-dev \
                    libprotobuf-lite32t64 \
                    libprotoc32t64
```

2. Build v1.60.0 gRPC C++ library from sources. Make sure to use 
```
git clone --recurse-submodules -b v1.60.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
```
to clone instead of command provided in guide

3. Make newly installed gRPC binaries visible:
```
export PATH=~/.local/bin:$PATH
```

4. Codegen sources from protobufs
```
protoc --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` rpc_sync.proto
```

5. Build with CMake
```
cmake -B build -S .
cmake --build build
```

6. Run in 2 separate terminals (or with `&` but be prepared to unfriendly logs)
```
./build/app 1234 4321 sleep
./build/app 4321 1234 run
```
