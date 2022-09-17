set (CMAKE_CXX_STANDARD 20)

add_executable(helloworldserver main.cpp)

target_link_libraries(helloworldserver PRIVATE wingrpc libprotobuf)

# maybe this is needed.
#add_dependencies(helloworldserver protobuf::protoc)

#download the proto from grpc example
set(_proto_file_path ${CMAKE_CURRENT_BINARY_DIR}/helloworld.proto)
if(NOT EXISTS ${_proto_file_path})
    message(STATUS "downloading helloworld proto")
    file(DOWNLOAD
        https://raw.githubusercontent.com/grpc/grpc/master/examples/protos/helloworld.proto
        ${_proto_file_path}
    )
endif()

# generate command is inside find module
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${_proto_file_path}
#    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>"
#    PLUGIN_OPTIONS --grpc_out ${CMAKE_CURRENT_BINARY_DIR}
)
protobuf_generate(LANGUAGE grpc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:win_grpc_cpp_plugin>"
    #PLUGIN "protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>"
    OUT_VAR GRPC_SRCS
    APPEND_PATH
    GENERATE_EXTENSIONS
        .win_grpc.pb.h
        .win_grpc.pb.cc
    PROTOS ${_proto_file_path}
)


target_sources(helloworldserver PRIVATE ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS})

target_include_directories(helloworldserver
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)