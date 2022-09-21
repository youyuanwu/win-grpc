#include "boost/asio.hpp"
#include "boost/winasio/winhttp/winhttp.hpp"

#include "boost/winasio/winhttp/client.hpp"

#include "client_stub.hpp"

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

  // set to http2 version
  const DWORD enableHTTP2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;
  h_session.set_option(WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                       (PVOID)&enableHTTP2Flag, sizeof(enableHTTP2Flag), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for http2 failed: " << ec;
    return EXIT_FAILURE;
  }

  // This is need to receiveing trailer
  DWORD stream_end = 1; // set to true
  h_session.set_option(WINHTTP_OPTION_REQUIRE_STREAM_END, (PVOID)&stream_end,
                       sizeof(stream_end), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for stream end failed: " << ec;
    return EXIT_FAILURE;
  }

  const DWORD tlsProtocols =
      WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
  h_session.set_option(WINHTTP_OPTION_SECURE_PROTOCOLS, (PVOID)&tlsProtocols,
                       sizeof(tlsProtocols), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for tlsProtocols failed: " << ec;
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

  client c(h_connect);

  helloworld::HelloRequest request;
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