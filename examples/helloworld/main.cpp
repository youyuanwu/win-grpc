#include "boost/winasio/http/http.hpp"
#include "boost/winasio/http/temp.hpp"
#include "helloworld.pb.h"

namespace net = boost::asio;
namespace winnet = boost::winasio;

void diagnosepayload(std::string body) {
  bool compress = body[0];
  std::size_t len = (int)body[4] | (int)body[3] << 8 | (int)body[2] << 16 |
                    (int)body[1] << 24;
  std::cout << "totallen" << body.length() << std::endl;
  std::cout << "diag is compressed " << compress << std::endl;
  std::cout << "diag len is " << len << std::endl;
}

int main() {

  // init http module
  winnet::http::http_initializer init;
  // add https then this becomes https server
  std::wstring url = L"https://localhost:12356/";

  boost::system::error_code ec;
  net::io_context io_context;

  // open queue handle
  winnet::http::basic_http_handle<net::io_context::executor_type> queue(
      io_context);
  queue.assign(winnet::http::open_raw_http_queue());
  winnet::http::http_simple_url simple_url(queue, url);

  auto handler = [](const winnet::http::simple_request &request,
                    winnet::http::simple_response &response) {
    // grpc always send success in http headers.
    response.set_status_code(200);
    response.set_reason("OK");
    response.set_content_type("application/grpc+proto");

    // PHTTP_REQUEST req = request.get_request();

    std::string body = request.get_body_string();
    // check len? len is big endian
    if (body.size() < 5) {
      std::cout << "body too small" << std::endl;
      response.add_trailer("grpc-status", "3"); // invalid argument
      response.add_trailer("grpc-message",
                           "body-too-small"); // TODO: percent encode
      return;
    }

    bool compress = body[0];
    std::size_t len = (int)body[4] | (int)body[3] << 8 | (int)body[2] << 16 |
                      (int)body[1] << 24;
    // std::cout << "is compressed " << compress << std::endl;
    // std::cout << "len is " << len << std::endl;
    if (body.size() - 5 != len) {
      response.add_trailer("grpc-status", "3"); // invalid argument
      response.add_trailer("grpc-message",
                           "body-len-not-match"); // TODO: percent encode
      std::cout << "is compressed " << compress << std::endl;
      std::cout << "len is " << len << "size is " << body.size() << std::endl;
      return;
    }
    std::string msg(body.begin() + 5, body.end());
    helloworld::HelloRequest req;
    bool ok = req.ParseFromString(msg);
    if (!ok) {
      response.add_trailer("grpc-status", "3"); // invalid argument
      response.add_trailer("grpc-message",
                           "cannot-parse-proto"); // TODO: percent encode
      return;
    }
    std::cout << "recieved hello.name: " << req.name() << std::endl;

    helloworld::HelloReply reply;
    reply.set_message("hello " + req.name());
    std::string encoded_reply = reply.SerializeAsString();
    std::int32_t reply_len = static_cast<std::int32_t>(encoded_reply.size());
    std::vector<BYTE> meta(4, 0);
    int j = 3;
    for (int i = 0; i <= 3; i++) {
      meta[j] = (reply_len >> (8 * i)) & 0xff;
      j--;
    }
    std::string reply_body(1, (BYTE)0); // size and char
    reply_body += std::string(meta.begin(), meta.end());
    reply_body += encoded_reply;
    // diagnosepayload(reply_body);

    response.set_body(reply_body);
    response.add_trailer("grpc-status", "0"); // OK
  };

  std::make_shared<http_connection<net::io_context::executor_type>>(queue,
                                                                    handler)
      ->start();

  io_context.run();
}