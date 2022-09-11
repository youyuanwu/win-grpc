set (CMAKE_CXX_STANDARD 20)

add_executable(helloworldserver main.cpp)

target_link_libraries(helloworldserver PRIVATE winasio libprotobuf)

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

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${_proto_file_path})

target_sources(helloworldserver PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(helloworldserver
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)