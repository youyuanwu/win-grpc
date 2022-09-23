#include "boost/winasio/http/basic_http_url.hpp"
#include "boost/winasio/http/http.hpp"
#include "boost/winasio/http/temp.hpp"

#include "boost/wingrpc/wingrpc.hpp"

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

  server2 svr;

  grpc::ServiceMiddleware middl;
  middl.add_service(svr);

  // open queue handle
  winnet::http::basic_http_queue_handle<net::io_context::executor_type> queue(
      io_context);
  queue.assign(winnet::http::open_raw_http_queue());
  winnet::http::basic_http_url<net::io_context::executor_type> simple_url(queue,
                                                                          url);

  auto handler = std::bind(grpc::default_handler, middl, std::placeholders::_1,
                           std::placeholders::_2);

  std::make_shared<http_connection<net::io_context::executor_type>>(queue,
                                                                    handler)
      ->start();

  io_context.run();
}