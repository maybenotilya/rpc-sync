# RPC sync

2 processes syncronization via gRPC communication.

# How to run
1. Build sources from protobufs
```
protoc --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` rpc_sync.proto
```

2. Build with CMake
```
cmake -B build -S .
cmake --build build
```

3. Run in 2 separate terminals (or with `&` but be prepared to unfriendly logs)
```
./build/app 1234 4321 sleep
./build/app 4321 1234 run
```
