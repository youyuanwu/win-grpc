set (CMAKE_CXX_STANDARD 20) # require std::optional etc

set(_server_exe helloworld_server)
add_executable(${_server_exe} main.cpp)

set(_client_exe helloworld_client)
add_executable(${_client_exe} client_main.cpp)

set(_shared_lib helloworld_lib)

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

# library that contains helloworld generated code
add_library(${_shared_lib} ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS})
target_include_directories(${_shared_lib}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(${_shared_lib} PUBLIC wingrpc libprotobuf)

target_link_libraries(${_server_exe} PRIVATE ${_shared_lib})
target_link_libraries(${_client_exe} PRIVATE ${_shared_lib})