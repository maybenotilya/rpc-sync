# RPC sync

2 processes syncronization via gRPC communication.

# How to run
1. Install `protoc` compiler and `grpc_cpp_plugin`. For example installation for Ubuntu:
```
sudo apt update && sudo apt upgrade
sudo apt install protobuf-compiler
sudo apt install protobuf-compiler-grpc
```

2. Build sources from protobufs
```
protoc --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` rpc_sync.proto
```

3. Build with CMake
```
cmake -B build -S .
cmake --build build
```

4. Run in 2 separate terminals (or with `&` but be prepared to unfriendly logs)
```
./build/app 1234 4321 sleep
./build/app 4321 1234 run
```
