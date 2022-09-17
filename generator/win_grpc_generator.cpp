#include "win_grpc_generator.hpp"

namespace win_grpc_generator {


inline bool ClientOnlyStreaming(const grpc_generator::Method* method) {
  return method->ClientStreaming() && !method->ServerStreaming();
}

inline bool ServerOnlyStreaming(const grpc_generator::Method* method) {
  return !method->ClientStreaming() && method->ServerStreaming();
}

grpc::string GetHeaderPrologue(grpc_generator::File* file){
   grpc::string output;
  {
    // Scope the output stream so it closes and finalizes output to the string.
    auto printer = file->CreatePrinter(&output);
    std::map<grpc::string, grpc::string> vars;

    vars["filename"] = file->filename();
    vars["filename_base"] = file->filename_without_ext();
    vars["message_header_ext"] = ".pb.h";

    printer->Print(vars, "// Generated by the win-grpc C++ plugin.\n");
    printer->Print(vars,
                   "// If you make any local change, they will be lost.\n");
    printer->Print(vars, "// source: $filename$\n");
    grpc::string leading_comments = file->GetLeadingComments("//");
    if (!leading_comments.empty()) {
      printer->Print(vars, "// Original file comments:\n");
      printer->PrintRaw(leading_comments.c_str());
    }
    printer->Print(vars, "#pragma once\n");
    // protobuf header
    printer->Print(vars, "#include \"$filename_base$$message_header_ext$\"\n");
    printer->Print(vars, "#include \"boost/system/error_code.hpp\"\n");
    printer->Print(vars, "#include \"boost/wingrpc/wingrpc.hpp\"\n");
    printer->Print(vars, file->additional_headers().c_str());
    printer->Print(vars, "\n");
  }
  return output;
}

void PrintHeaderServerMethodSync(grpc_generator::Printer* printer,
                                 const grpc_generator::Method* method,
                                 std::map<grpc::string, grpc::string>* vars) {
  (*vars)["Method"] = method->name();
  (*vars)["Request"] = method->input_type_name();
  (*vars)["Response"] = method->output_type_name();
  printer->Print(method->GetLeadingComments("//").c_str());
  if (method->NoStreaming()) {
    printer->Print(*vars,
                   "virtual void $Method$("
                   "boost::system::error_code &ec, const $Request$* request, "
                   "$Response$* response) = 0;\n");
  } else if (ClientOnlyStreaming(method)) {
    printer->Print(*vars,
      "// ClientOnlyStreaming for method $Method$ request $Request$ response $Response$ not supported\n");
  } else if (ServerOnlyStreaming(method)) {
    printer->Print(*vars,
      "// ServerOnlyStreaming for method $Method$ request $Request$ response $Response$ not supported\n");
  } else if (method->BidiStreaming()) {
    printer->Print(
        *vars,
        "// BidiStreaming for method $Method$ request $Request$ response $Response$ not supported\n");
  }
  printer->Print(method->GetTrailingComments("//").c_str());
}

void PrintHeaderServerRouterApplyMethodEntry(grpc_generator::Printer* printer,
                                 const grpc_generator::Method* method,
                                 std::map<grpc::string, grpc::string>* vars){
  if (method->NoStreaming()) {
    printer->Print(*vars,
      "if(url == L\"/$Package$$Service$/$Method$\") { \n"
      "  auto op = std::bind(&Service::SayHello, this, " 
      "  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);\n"
      "  boost::wingrpc::handle_request<$Request$, $Response$>(ec, request, response, op); \n"
      "  return true;\n"
      "}\n");
  } else{
        printer->Print(*vars, "// Streaming for method $Method$ request $Request$ response $Response$ not supported\n");
  }
}

void PrintHeaderServerRouterApply(grpc_generator::Printer* printer,
                        const grpc_generator::Service* service,
                        std::map<grpc::string, grpc::string>* vars){
    // bool returns if route is found
    printer->Print(*vars,
      "bool HandleRequest(boost::system::error_code &ec, std::wstring const & url, std::string const & request, std::string & response) override {\n");
    printer->Indent();
    for (int i = 0; i < service->method_count(); ++i) {
      PrintHeaderServerRouterApplyMethodEntry(printer, service->method(i).get(), vars);
    }
    printer->Print(*vars,"return false;\n");
    printer->Outdent();
    printer->Print(*vars,
      "}\n"
      );
}

void PrintHeaderService(grpc_generator::Printer* printer,
                        const grpc_generator::Service* service,
                        std::map<grpc::string, grpc::string>* vars) {
  (*vars)["Service"] = service->name();

  printer->Print(service->GetLeadingComments("//").c_str());
  printer->Print(*vars,
                 "class $Service$ final {\n"
                 " public:\n");
  printer->Indent();

  // Service metadata
  printer->Print(*vars,
                 "static constexpr char const* service_full_name() {\n"
                 "  return \"$Package$$Service$\";\n"
                 "}\n");

    // Server side - base
  printer->Print(
      "class Service : public boost::wingrpc::Service {\n"
      " public:\n");
  printer->Indent();
  printer->Print("Service() {};\n");
  printer->Print("virtual ~Service() {};\n");
  for (int i = 0; i < service->method_count(); ++i) {
    PrintHeaderServerMethodSync(printer, service->method(i).get(), vars);
  }

  printer->Print("\n");
  PrintHeaderServerRouterApply(printer, service, vars);

  printer->Outdent();
  printer->Print("};\n");

  printer->Outdent();
  printer->Print(*vars,"};\n");
}


grpc::string GetHeaderServices(grpc_generator::File* file){
  grpc::string output;
  {
    auto printer = file->CreatePrinter(&output);
    std::map<grpc::string, grpc::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    for (int i = 0; i < file->service_count(); ++i) {
      PrintHeaderService(printer.get(), file->service(i).get(), &vars);
      printer->Print("\n");
    }
  }
  return output;
}


} // namespace win_grpc_generator