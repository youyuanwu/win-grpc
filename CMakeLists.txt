cmake_minimum_required(VERSION 3.14)

project(win-grpc)

set(wingrpc_MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(wingrpc_MAIN_PROJECT ON)
endif()

option(wingrpc_BuildTests     "Build the unit tests when BUILD_TESTING is enabled." ${winasio_MAIN_PROJECT})
option(wingrpc_BuildExamples  "Build examples"                                      ${winasio_MAIN_PROJECT})

# import winext
message(STATUS "feching winasio")
include(FetchContent)
FetchContent_Declare(winasio
  GIT_REPOSITORY https://github.com/youyuanwu/winasio.git
  GIT_TAG 3ec686202443e9cd9cc667f6943ca77754a84b9a)
FetchContent_GetProperties(winasio)
if(NOT winasio_POPULATED)
  FetchContent_Populate(winasio)
endif()

if(${wingrpc_MAIN_PROJECT})
  # format
  include(${winasio_SOURCE_DIR}/cmake/clang-format.cmake)
endif(${wingrpc_MAIN_PROJECT})
if(WINASIO_CI_FORMAT)
  message(STATUS "Only added format target.")
  return()
endif()

add_subdirectory(${winasio_SOURCE_DIR} ${winasio_BINARY_DIR} EXCLUDE_FROM_ALL)

if (TARGET libprotobuf-lite)
  # If grpc build is already added in the tree, protobuf can be skipped.
  message(STATUS "Protobuf already added as target. Skip feching protobuf.")
else()
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
endif(TARGET libprotobuf-lite)

add_library(wingrpc INTERFACE)
target_include_directories(wingrpc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(wingrpc
    INTERFACE winasio
)

add_subdirectory(generator)

if(wingrpc_BuildExamples)
  add_subdirectory(examples/helloworld)
endif(wingrpc_BuildExamples)