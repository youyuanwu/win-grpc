#pragma once

#include "boost/winasio/http/convert.hpp" // simple_requests
#include "boost/wingrpc/wingrpc_common.hpp"
#include <boost/asio.hpp>
#include <boost/winasio/http/convert.hpp>
#include <boost/winasio/http/http.hpp>

namespace net = boost::asio;
namespace winnet = boost::winasio;

#include <string>

namespace boost {
namespace wingrpc {

namespace net = boost::asio; // from <boost/asio.hpp>
namespace winnet = boost::winasio;

// request_str should be moved into this.
template <typename Request, typename Reply>
inline void handle_request(
    boost::system::error_code &ec, std::string request_str,
    std::string &reply_str,
    std::function<void(boost::system::error_code &ec, const Request *, Reply *)>
        op) {
  Request req;
  parse_length_prefixed_message(ec, std::move(request_str), req);

  if (ec) {
    // response.add_trailer("grpc-status", "3"); // invalid argument
    // response.add_trailer("grpc-message", "invalid-body");
    return;
  }

  Reply reply;
  op(ec, &req, &reply);

  if (ec) {
    // TODO
    return;
  }

  encode_length_prefixed_message(reply, reply_str);
}

// grpc service
class Service {
public:
  // request needs to be moved into.
  virtual bool HandleRequest(boost::system::error_code &ec,
                             std::wstring const &url, std::string request,
                             std::string &response) = 0;
};

class ServiceMiddleware {
public:
  inline void add_service(Service &service) { services_.push_back(service); }

  // requset should be moved into this.
  inline bool handle_request(boost::system::error_code &ec,
                             std::wstring const &url, std::string request,
                             std::string &response) {
    // go through service one by one and dispatch
    for (auto &service : services_) {
      bool found =
          service.get().HandleRequest(ec, url, std::move(request), response);
      if (found) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<std::reference_wrapper<Service>> services_;
};

inline void default_handler(ServiceMiddleware &middl,
                            const winnet::http::simple_request &request,
                            winnet::http::simple_response &response) {
  boost::system::error_code ec;

  // todo: validate content type etc.

  // grpc always send success in http headers.
  response.set_status_code(200);
  response.set_reason("OK");
  response.set_content_type("application/grpc+proto");

  PHTTP_REQUEST req = request.get_request();
  BOOST_LOG_TRIVIAL(debug) << "http version " << req->Version.MajorVersion
                           << "." << req->Version.MinorVersion;
  BOOST_LOG_TRIVIAL(debug) << "url path is " << req->CookedUrl.pAbsPath;
  // BOOST_LOG_TRIVIAL(debug) << "request content:" << request;

  // make a copy of the request and move into parsing.
  // TODO: maybe the winasio should expose way to consume the body string.
  std::string request_str = request.get_body_string();
  std::string reply_str;
  std::wstring url = std::wstring(req->CookedUrl.pAbsPath);
  bool found = middl.handle_request(ec, url, std::move(request_str), reply_str);
  if (!found) {
    BOOST_LOG_TRIVIAL(debug) << "url not found " << url;
    response.add_trailer("grpc-status", "3"); // invalid
    response.add_trailer("grpc-message", "various-error");
    return;
  }

  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "handler has error " << ec;
    response.add_trailer("grpc-status", "3"); // invalid
    response.add_trailer("grpc-message", "various-error");
    return;
  }

  BOOST_LOG_TRIVIAL(debug) << "reply len is " << reply_str.size();
  response.set_body(reply_str);
  response.add_trailer("grpc-status", "0"); // OK
}

} // namespace wingrpc
} // namespace boost