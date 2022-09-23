#pragma once

#include "src/compiler/config.h"
#include "src/compiler/schema_interface.h"

namespace win_grpc_generator {

// class client_gen{
// public:
//   client_gen(grpc_generator::File & file): file_(file) {}

//   grpc::string content();

// private:

//   grpc_generator::File & file_;
// };

grpc::string GetClientHeaderServices(grpc_generator::File* file);


} // namespace win_grpc_generator