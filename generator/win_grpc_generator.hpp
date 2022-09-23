#pragma once

#include "src/compiler/config.h"
#include "src/compiler/schema_interface.h"

namespace win_grpc_generator
{

    // Return the prologue of the generated header file.
    grpc::string GetHeaderPrologue(grpc_generator::File * file);

    // Return the prologue of the generated source file.
    grpc::string GetSourcePrologue(grpc_generator::File * file);

    // Return the services for generated header file.
    grpc::string GetHeaderServices(grpc_generator::File * file);

    // Return the services for generated source file.
    grpc::string GetSourceServices(grpc_generator::File * file);

} // win_grpc_generator