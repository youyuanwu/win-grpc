#include "boost/asio.hpp"
#include "boost/winasio/winhttp/winhttp.hpp"

#include "boost/winasio/winhttp/client.hpp"

// #include "client_stub.hpp"
#include "helloworld.win_grpc.pb.h"

namespace net = boost::asio; // from <boost/asio.hpp>
namespace winnet = boost::winasio;

int main() {
  boost::system::error_code ec;
  net::io_context io_context;
  winnet::winhttp::basic_winhttp_session_handle<net::io_context::executor_type>
      h_session(io_context);
  h_session.open(ec); // by default async session is created
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "Session open failed: " << ec;
    return EXIT_FAILURE;
  }

  grpc::configure_grpc_session(h_session, ec);
  if (ec) {
    return EXIT_FAILURE;
  }

  winnet::winhttp::url_component url;
  url.crack(L"https://localhost:12356", ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "Crack URL failed: " << ec;
    return EXIT_FAILURE;
  }

  winnet::winhttp::basic_winhttp_connect_handle<net::io_context::executor_type>
      h_connect(io_context);
  h_connect.connect(h_session.native_handle(), url.get_hostname().c_str(),
                    url.get_port(), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "Connect failed: " << ec;
    return EXIT_FAILURE;
  }

  GreeterClient::Stub c(h_connect);

  helloworld::HelloRequest request;
  request.set_name("winhttp");
  // request.SerializePartialToArray
  // request.ByteSizeLong();

  c.SayHello(&request, [](boost::system::error_code ec,
                          const helloworld::HelloReply *response) {
    if (ec) {
      BOOST_LOG_TRIVIAL(debug) << "SayHello failed: " << ec;
      return;
    }
    BOOST_LOG_TRIVIAL(debug) << "SayHello Reply: " << response->message();
  });

  // c.SayHello(&request, [](boost::system::error_code ec,
  //                         const helloworld::HelloReply *response) {
  //   if(ec){
  //     BOOST_LOG_TRIVIAL(debug) << "SayHello2 failed: " << ec;
  //     return;
  //   }
  //   BOOST_LOG_TRIVIAL(debug) << "SayHello2 Reply: " << response->message();
  // });

  io_context.run();
}