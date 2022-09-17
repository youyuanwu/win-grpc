# grpc repo is needed to build generator plugin

message(STATUS "Fetching grpc")
include(FetchContent)

FetchContent_Declare(
  grpc
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.28.0
  GIT_SUBMODULES "" # no submodules
)
#set(FETCHCONTENT_QUIET OFF)

if(NOT grpc_POPULATED)
  FetchContent_Populate(grpc)
  #add_subdirectory(${protobuf_SOURCE_DIR} ${protobuf_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# see this for plugin files https://github.com/grpc/grpc/blob/master/src/compiler/BUILD
set(_grpc_compiler_dir ${grpc_SOURCE_DIR}/src/compiler)

set(_sources
"config.h"
"config_protobuf.h"
"cpp_generator.h"
"cpp_generator_helpers.h"
"cpp_plugin.h"
"generator_helpers.h"
"protobuf_plugin.h"
"schema_interface.h"

"cpp_generator.cc"

"cpp_plugin.cc"
)

foreach (_src ${_sources})
    list (APPEND SRCS_ABS "${_grpc_compiler_dir}/${_src}")
endforeach()

add_executable(grpc_cpp_plugin 
  ${SRCS_ABS}
)

target_include_directories(grpc_cpp_plugin
  PRIVATE ${grpc_SOURCE_DIR} ${grpc_SOURCE_DIR}/include
)

target_link_libraries(grpc_cpp_plugin PRIVATE libprotobuf libprotoc)

# protobuf-src\src\google\protobuf\compiler\plugin.cc is used by the plugin

# win generator

set(WIN_SOURCES
win_cpp_plugin.cpp
win_cpp_plugin.hpp
win_grpc_generator.cpp
win_grpc_generator.hpp
)

add_executable(win_grpc_cpp_plugin 
  ${WIN_SOURCES}
)

target_include_directories(win_grpc_cpp_plugin
  PRIVATE . 
  PRIVATE ${grpc_SOURCE_DIR} ${grpc_SOURCE_DIR}/include
)

target_link_libraries(win_grpc_cpp_plugin PRIVATE libprotobuf libprotoc)