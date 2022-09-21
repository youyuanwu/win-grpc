#pragma once

#include "helloworld.pb.h"
#include "boost/system/error_code.hpp"
#include "boost/wingrpc/wingrpc_client.hpp"


#include <functional>

namespace net = boost::asio;
namespace winnet = boost::winasio;
namespace grpc = boost::wingrpc;

// holds data that persists during async call
template<typename Executor>
class request_holder {
public:
  typedef Executor executor_type;
  request_holder(const executor_type &ex): h_request_(ex), pl_(), body_buff_(), dynamic_body_buff_(body_buff_){}

// private:
  winnet::winhttp::basic_winhttp_request_asio_handle<executor_type> h_request_;
  winnet::winhttp::payload pl_;

  // buffer for response
  std::vector<BYTE> body_buff_;
  net::dynamic_vector_buffer<BYTE, std::allocator<BYTE>> dynamic_body_buff_;
};

template<typename Executor>
class client{
public:
  //type
  typedef Executor executor_type;

  client(winnet::winhttp::basic_winhttp_connect_handle<executor_type> & h_connect) : h_connect_(h_connect) {}

  void SayHello(::helloworld::HelloRequest* request, std::function<void(boost::system::error_code ec,  const ::helloworld::HelloReply* response)> token)
  {
    std::string request_payload;
    grpc::encode_length_prefixed_message(*request, request_payload);

    std::shared_ptr<request_holder<executor_type>> req = std::make_shared<request_holder<executor_type>>(h_connect_.get_executor());

    winnet::winhttp::payload & pl = req->pl_; 
    pl.method = L"POST";
    pl.path = L"/helloworld.Greeter/SayHello";
    //pl.header = std::make_optional<winnet::winhttp::header::headers>();
    //pl.header->add()
    pl.accept = std::make_optional<winnet::winhttp::header::accept_types>({L"application/grpc+proto"});
    pl.body = std::move(request_payload);
    pl.secure = true;

    winnet::winhttp::async_exec(
      pl, h_connect_, req->h_request_, req->dynamic_body_buff_,
      [req, token = std::move(token)]
      (boost::system::error_code ec, std::size_t) mutable {
        BOOST_LOG_TRIVIAL(debug) << "async_exec handler";
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "Hanlder error" << ec;
          return;
        }

        // print result
        std::wstring headers;
        winnet::winhttp::header::get_all_raw_crlf(req->h_request_, ec, headers);
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "fail to get headers" << ec;
          return;
        }
        std::wstring trailers;
        winnet::winhttp::header::get_trailers(req->h_request_, ec, trailers);
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "fail to get trailers " << ec;
          return;
        }
        BOOST_LOG_TRIVIAL(debug) << "trailers: " << trailers;
        token(ec, nullptr);
    });

  }
private:
  winnet::winhttp::basic_winhttp_connect_handle<executor_type> & h_connect_;
};