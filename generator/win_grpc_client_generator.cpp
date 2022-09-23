#include "win_grpc_client_generator.hpp"
#include "win_grpc_genrator_common.hpp"


namespace win_grpc_generator {

void PrintHeaderClientMethodSync(grpc_generator::Printer* printer,
                                 const grpc_generator::Method* method,
                                 std::map<grpc::string, grpc::string>* vars) {
  (*vars)["Method"] = method->name();
  (*vars)["Request"] = method->input_type_name();
  (*vars)["Response"] = method->output_type_name();
  printer->Print(method->GetLeadingComments("//").c_str());
  if (method->NoStreaming()) {
    printer->Print(*vars,
      "void $Method$($Request$ *request,\n"
      "             std::function<void(boost::system::error_code ec,\n"
      "                               const $Response$ *response)>\n"
      "  token) {\n"
      "    std::shared_ptr<grpc::request_holder<executor_type>> req =\n"
      "    std::make_shared<grpc::request_holder<executor_type>>(\n"
      "    h_connect_.get_executor());\n"
      "    req->set_path(L\"/$Package$$Service$/$Method$\");\n"
      "    grpc::client_exec<executor_type>(req, h_connect_, request, token);\n"
      "}\n"
    );
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

void PrintClientHeaderService(grpc_generator::Printer* printer,
                        const grpc_generator::Service* service,
                        std::map<grpc::string, grpc::string>* vars) {
  (*vars)["Service"] = service->name();

  printer->Print(service->GetLeadingComments("//").c_str());
  printer->Print(*vars,
                 "class $Service$Client final {\n"
                 " public:\n");
  printer->Indent();
  
  // client stub
  printer->Print(
      "template <typename Executor> class Stub {\n"
      " public:\n");
  printer->Indent();

  printer->Print(
    "typedef Executor executor_type;\n"
    "Stub(\n"
    "winnet::winhttp::basic_winhttp_connect_handle<executor_type> &h_connect)\n"
    ": h_connect_(h_connect) {}\n"
  );

  for (int i = 0; i < service->method_count(); ++i) {
    PrintHeaderClientMethodSync(printer, service->method(i).get(), vars);
  }
  printer->Print("\n");

  printer->Outdent();
  printer->Print("private:\n");
  printer->Indent();
  printer->Print("winnet::winhttp::basic_winhttp_connect_handle<executor_type> &h_connect_;\n");
  
  // close for stub
  printer->Outdent();
  printer->Print("};\n");

  // close for service
  printer->Outdent();
  printer->Print(*vars,"};\n");
}



grpc::string GetClientHeaderServices(grpc_generator::File* file){
  grpc::string output;
  {
    auto printer = file->CreatePrinter(&output);
    std::map<grpc::string, grpc::string> vars;

    vars["Package"] = file->package();
    if (!file->package().empty()) {
      vars["Package"].append(".");
    }

    printer->Print(
      "#include \"boost/wingrpc/wingrpc_client.hpp\"\n"
      "namespace net = boost::asio;\n"
      "namespace winnet = boost::winasio;\n"
      "namespace grpc = boost::wingrpc;\n"
    );

    for (int i = 0; i < file->service_count(); ++i) {
      PrintClientHeaderService(printer.get(), file->service(i).get(), &vars);
      printer->Print("\n");
    }
  }
  return output;
}


} // win_grpc_generator