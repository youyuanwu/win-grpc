#include "boost/asio.hpp"
#include "boost/winasio/winhttp/winhttp.hpp"

#include "boost/winasio/winhttp/client.hpp"

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

  // DWORD http2required = 1;
  // h_session.set_option(WINHTTP_OPTION_HTTP_PROTOCOL_REQUIRED,
  // (PVOID)&http2required,
  //                      sizeof(http2required), ec);
  // if (ec) {
  //   BOOST_LOG_TRIVIAL(debug) << L"set_option for http2required failed: " <<
  //   ec; return EXIT_FAILURE;
  // }

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

  winnet::winhttp::payload pl;
  pl.method = L"GET";
  pl.path = std::nullopt;
  pl.header = std::nullopt;
  pl.accept = std::nullopt;
  pl.body = std::nullopt;
  pl.secure = true;

  winnet::winhttp::basic_winhttp_request_asio_handle<
      net::io_context::executor_type>
      h_request(io_context.get_executor());

  std::vector<BYTE> body_buff;
  auto buff = net::dynamic_buffer(body_buff);
  winnet::winhttp::async_exec(
      pl, h_connect, h_request, buff,
      [&h_request, &buff](boost::system::error_code ec, std::size_t) {
        BOOST_LOG_TRIVIAL(debug) << "async_exec handler";
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "Hanlder error" << ec;
          return;
        }

        // print result
        std::wstring headers;
        winnet::winhttp::header::get_all_raw_crlf(h_request, ec, headers);
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "fail to get headers" << ec;
          return;
        }
        BOOST_LOG_TRIVIAL(debug) << headers;
        BOOST_LOG_TRIVIAL(debug) << winnet::winhttp::buff_to_string(buff);

        // more tests
        // check status
        DWORD dwStatusCode;
        winnet::winhttp::header::get_status_code(h_request, ec, dwStatusCode);
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "fail to get status code" << ec;
          return;
        }

        std::wstring trailers;
        winnet::winhttp::header::get_trailers(h_request, ec, trailers);
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "fail to get trailers " << ec;
          return;
        }
        BOOST_LOG_TRIVIAL(debug) << "trailers: " << trailers;
      });

  io_context.run();
}