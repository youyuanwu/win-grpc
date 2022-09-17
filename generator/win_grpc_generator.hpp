#pragma once

#include "src/compiler/config.h"
#include "src/compiler/schema_interface.h"

namespace win_grpc_generator {

grpc::string GetHeaderPrologue(grpc_generator::File* file);

grpc::string GetHeaderServices(grpc_generator::File* file);

} // win_grpc_generator