cmake_minimum_required(VERSION 3.15)
project(RPCSync)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;/usr/local")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/grpc")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/protobuf")
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROTO_FILES "${PROTO_PATH}/rpc_sync.proto")

set(PROTO_SRCS "${PROTO_PATH}/rpc_sync.pb.cc")
set(PROTO_HDRS "${PROTO_PATH}/rpc_sync.pb.h")
set(GRPC_SRCS "${PROTO_PATH}/rpc_sync.grpc.pb.cc")
set(GRPC_HDRS "${PROTO_PATH}/rpc_sync.grpc.pb.h")

add_executable(app 
    main.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(app
    gRPC::grpc++
    protobuf::libprotobuf
    Threads::Threads
)
