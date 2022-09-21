#pragma once

#include "boost/system/error_code.hpp"
#include "boost/wingrpc/wingrpc_client.hpp"
#include "helloworld.pb.h"

#include <functional>

namespace net = boost::asio;
namespace winnet = boost::winasio;
namespace grpc = boost::wingrpc;

template <typename Executor> class client {
public:
  // type
  typedef Executor executor_type;

  client(
      winnet::winhttp::basic_winhttp_connect_handle<executor_type> &h_connect)
      : h_connect_(h_connect) {}

  void SayHello(::helloworld::HelloRequest *request,
                std::function<void(boost::system::error_code ec,
                                   const ::helloworld::HelloReply *response)>
                    token) {
    std::shared_ptr<grpc::request_holder<executor_type>> req =
        std::make_shared<grpc::request_holder<executor_type>>(
            h_connect_.get_executor());
    req->set_path(L"/helloworld.Greeter/SayHello");

    grpc::client_exec<executor_type>(req, h_connect_, request, token);
  }

private:
  winnet::winhttp::basic_winhttp_connect_handle<executor_type> &h_connect_;
};