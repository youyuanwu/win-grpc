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
  if(ec){
    BOOST_LOG_TRIVIAL(debug) << "Session open failed: " << ec;
    return EXIT_FAILURE;
  }

  winnet::winhttp::url_component url;
  url.crack(L"http://localhost:12356", ec);
  if(ec){
    BOOST_LOG_TRIVIAL(debug) << "Crack URL failed: " << ec;
    return EXIT_FAILURE;
  }

  winnet::winhttp::basic_winhttp_connect_handle<net::io_context::executor_type>
      h_connect(io_context);
  h_connect.connect(h_session.native_handle(), url.get_hostname().c_str(),
                    url.get_port(), ec);
  if(ec){
    BOOST_LOG_TRIVIAL(debug) << "Connect failed: " << ec;
    return EXIT_FAILURE;
  }

  winnet::winhttp::payload pl;
  pl.method = L"GET";
  pl.path = std::nullopt;
  pl.header = std::nullopt;
  pl.accept = std::nullopt;
  pl.body = std::nullopt;
  pl.secure = false;

  winnet::winhttp::basic_winhttp_request_asio_handle<
      net::io_context::executor_type>
      h_request(io_context.get_executor());

  std::vector<BYTE> body_buff;
  auto buff = net::dynamic_buffer(body_buff);
  winnet::winhttp::async_exec(
      pl, h_connect, h_request, buff,
      [&h_request, &buff](boost::system::error_code ec, std::size_t) {
        BOOST_LOG_TRIVIAL(debug) << "async_exec handler";
        if(ec){
          BOOST_LOG_TRIVIAL(debug) << "Hanlder error" << ec;
          return;
        }

        // print result
        std::wstring headers;
        winnet::winhttp::header::get_all_raw_crlf(h_request, ec, headers);
        if(ec){
          BOOST_LOG_TRIVIAL(debug) << "fail to get headers" << ec;
          return;
        }
        BOOST_LOG_TRIVIAL(debug) << headers;
        BOOST_LOG_TRIVIAL(debug) << winnet::winhttp::buff_to_string(buff);

        // more tests
        // check status
        DWORD dwStatusCode;
        winnet::winhttp::header::get_status_code(h_request, ec, dwStatusCode);
        if(ec){
          BOOST_LOG_TRIVIAL(debug) << "fail to get status code" << ec;
          return;
        }
      });

  io_context.run();

}