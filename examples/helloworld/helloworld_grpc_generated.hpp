#pragma once

#include "boost\wingrpc\wingrpc.hpp"

namespace grpc = boost::wingrpc;

// this should be auto generated
class Ihelloword_handler {
public:
  virtual void SayHello(boost::system::error_code &ec,
                        const helloworld::HelloRequest &request,
                        helloworld::HelloReply &reply);
};

class helloword_router {
public:
  helloword_router(grpc::router &rt) : rt_(rt) {}

  // server needs to be valide until shutdown.
  void init(Ihelloword_handler &svr) {
    auto op =
        std::bind(&Ihelloword_handler::SayHello, &svr, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3);
    rt_.add_operation<helloworld::HelloRequest, helloworld::HelloReply>(
        L"/helloworld.Greeter/SayHello", op);
  }

  // cannot return the fn because the return func signature may vary
  // we do not want to caller to specify template arg
  void dispatch(boost::system::error_code &ec, std::wstring url,
                const std::string request_str, std::string &reply_str) {

    // this is not optimal
    if (url == L"/helloworld.Greeter/SayHello") {
      rt_.dispatch<helloworld::HelloRequest, helloworld::HelloReply>(
          ec, url, request_str, reply_str);
    } else {
      BOOST_LOG_TRIVIAL(debug) << "fail to resolve_operation " << url;
      ec = boost::system::errc::make_error_code(
          boost::system::errc::not_supported);
    }
  }

  grpc::router &rt_;
};