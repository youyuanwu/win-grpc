#pragma once
#include "src/compiler/config.h"
#include "src/compiler/schema_interface.h"

namespace win_grpc_generator {


inline bool ClientOnlyStreaming(const grpc_generator::Method* method) {
  return method->ClientStreaming() && !method->ServerStreaming();
}

inline bool ServerOnlyStreaming(const grpc_generator::Method* method) {
  return !method->ClientStreaming() && method->ServerStreaming();
}

} // namespace win_grpc_generator