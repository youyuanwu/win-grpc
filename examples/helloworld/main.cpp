#include "boost/winasio/http/http.hpp"
#include "boost/winasio/http/temp.hpp"

#include "boost/wingrpc/wingrpc.hpp"

#include "helloworld.pb.h"

#include "helloworld_grpc_generated.hpp"
#include "helloworld_impl.hpp"

namespace net = boost::asio;
namespace winnet = boost::winasio;
namespace grpc = boost::wingrpc;

int main() {

  // init http module
  winnet::http::http_initializer init;
  // add https then this becomes https server
  std::wstring url = L"https://localhost:12356/";

  boost::system::error_code ec;
  net::io_context io_context;

  server svr;

  grpc::router rt;
  helloword_router h_rt(rt);
  h_rt.init(svr);

  // open queue handle
  winnet::http::basic_http_handle<net::io_context::executor_type> queue(
      io_context);
  queue.assign(winnet::http::open_raw_http_queue());
  winnet::http::http_simple_url simple_url(queue, url);

  auto handler = [svr, h_rt](const winnet::http::simple_request &request,
                             winnet::http::simple_response &response) mutable {
    boost::system::error_code ec;

    // todo: validate content type etc.

    // grpc always send success in http headers.
    response.set_status_code(200);
    response.set_reason("OK");
    response.set_content_type("application/grpc+proto");

    PHTTP_REQUEST req = request.get_request();
    BOOST_LOG_TRIVIAL(debug) << "url path is " << req->CookedUrl.pAbsPath;

    std::string request_str = request.get_body_string();
    std::string reply_str;
    h_rt.dispatch(ec, std::wstring(req->CookedUrl.pAbsPath), request_str,
                  reply_str);

    // error todo:
    if (ec) {
      response.add_trailer("grpc-status", "3"); // invalid
      response.add_trailer("grpc-message", "various-error");
      return;
    }

    BOOST_LOG_TRIVIAL(debug) << "reply len is " << reply_str.size();
    response.set_body(reply_str);
    response.add_trailer("grpc-status", "0"); // OK
  };

  std::make_shared<http_connection<net::io_context::executor_type>>(queue,
                                                                    handler)
      ->start();

  io_context.run();
}