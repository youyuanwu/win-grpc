#pragma once 

#include "src/compiler/generator_helpers.h"
#include "src/compiler/protobuf_plugin.h"
#include "win_grpc_generator.hpp"
#include "win_grpc_client_generator.hpp"

class WinCppGrpcGenerator : public grpc::protobuf::compiler::CodeGenerator {
 public:
  WinCppGrpcGenerator() {}
  virtual ~WinCppGrpcGenerator() {}

  virtual bool Generate(const grpc::protobuf::FileDescriptor* file,
                        const grpc::string& parameter,
                        grpc::protobuf::compiler::GeneratorContext* context,
                        grpc::string* error) const {
    if (file->options().cc_generic_services()) {
      *error =
          "cpp grpc proto compiler plugin does not work with generic "
          "services. To generate cpp grpc APIs, please set \""
          "cc_generic_service = false\".";
      return false;
    }

    ProtoBufFile pbfile(file);

    grpc::string file_name = grpc_generator::StripProto(file->name());
    std::unique_ptr<grpc::protobuf::io::ZeroCopyOutputStream> header_output(
        context->Open(file_name + ".win_grpc.pb.h"));
    grpc::protobuf::io::CodedOutputStream header_coded_out(header_output.get());
    grpc::string header_code = win_grpc_generator::GetHeaderPrologue(&pbfile) +
      win_grpc_generator::GetHeaderServices(&pbfile) +
      "// client content:\n"
      + win_grpc_generator::GetClientHeaderServices(&pbfile);
    header_coded_out.WriteRaw(header_code.data(), header_code.size());

    std::unique_ptr<grpc::protobuf::io::ZeroCopyOutputStream> source_output(
    context->Open(file_name + ".win_grpc.pb.cc"));
    grpc::protobuf::io::CodedOutputStream source_coded_out(source_output.get());
    std::string source_code = win_grpc_generator::GetSourcePrologue(&pbfile) +
      win_grpc_generator::GetSourceServices(&pbfile);
    source_coded_out.WriteRaw(source_code.data(), source_code.size());

    return true;
  }
};