cmake_minimum_required(VERSION 3.14)

project(win-grpc)

# import winext
message(STATUS "feching winasio")
include(FetchContent)
FetchContent_Declare(winasio
  GIT_REPOSITORY https://github.com/youyuanwu/winasio.git
  GIT_TAG 3ec686202443e9cd9cc667f6943ca77754a84b9a)
FetchContent_GetProperties(winasio)
if(NOT winasio_POPULATED)
  FetchContent_Populate(winasio)
  add_subdirectory(${winasio_SOURCE_DIR} ${winasio_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# import protobuf
message(STATUS "fetching protobuf")
FetchContent_Declare(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG        v3.21.3
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
if(NOT protobuf_POPULATED)
  FetchContent_Populate(protobuf)
  add_subdirectory(${protobuf_SOURCE_DIR} ${protobuf_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
#protoc-21.3-win64.zip

message(STATUS ${protobuf_BINARY_DIR})

set(Protobuf_LIBRARIES ${protobuf_BINARY_DIR})
set(Protobuf_INCLUDE_DIR ${protobuf_SOURCE_DIR}/src)
include(FindProtobuf)
find_package(Protobuf REQUIRED)

# format
include(${winasio_SOURCE_DIR}/cmake/clang-format.cmake)
if(WINASIO_CI_FORMAT)
    message(STATUS "Only added format target.")
    return()
endif()

add_subdirectory(examples/helloworld)