#include "win_cpp_plugin.hpp"

// #include "src/compiler/cpp_plugin.h"

int main(int argc, char * argv[])
{
    WinCppGrpcGenerator generator;
    return grpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
