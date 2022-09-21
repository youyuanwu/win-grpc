#pragma once

#include <string>

namespace boost {
namespace wingrpc {

// msg should be moved into moved.
// parse request into proto type
template <typename ProtoIn>
inline void parse_length_prefixed_message(boost::system::error_code &ec,
                                          std::string msg, ProtoIn &proto) {
  if (msg.size() < 5) {
    BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message msg too small";
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }

  bool compress = msg[0];
  std::size_t len =
      (int)msg[4] | (int)msg[3] << 8 | (int)msg[2] << 16 | (int)msg[1] << 24;
  if (msg.size() - 5 != len) {
    BOOST_LOG_TRIVIAL(debug)
        << "parse_length_prefixed_message msg size does not match";
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }
  std::string msg_body(msg.begin() + 5, msg.end());
  bool ok = proto.ParseFromString(msg_body);
  if (!ok) {
    BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message bad proto msg";
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }
}

template <typename ProtoReply>
void encode_length_prefixed_message(const ProtoReply &reply,
                                    std::string &msgout) {
  std::string encoded_reply = reply.SerializeAsString();
  std::int32_t reply_len = static_cast<std::int32_t>(encoded_reply.size());
  std::vector<BYTE> meta(4, 0); // encode 4 bytes big endian for length
  int j = 3;
  for (int i = 0; i <= 3; i++) {
    meta[j] = (reply_len >> (8 * i)) & 0xff;
    j--;
  }
  msgout = std::string(1, (BYTE)0); // size and char
  msgout += std::string(meta.begin(), meta.end());
  msgout += encoded_reply;
}

template <typename Request, typename Reply>
inline void handle_request(
    boost::system::error_code &ec, const std::string &request_str,
    std::string &reply_str,
    std::function<void(boost::system::error_code &ec, const Request *, Reply *)>
        op) {
  Request req;
  parse_length_prefixed_message(ec, request_str, req);

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
  virtual bool HandleRequest(boost::system::error_code &ec,
                             std::wstring const &url,
                             std::string const &request,
                             std::string &response) = 0;
};

class ServiceMiddleware {
public:
  inline void add_service(Service &service) { services_.push_back(service); }
  inline bool handle_request(boost::system::error_code &ec,
                             std::wstring const &url,
                             std::string const &request,
                             std::string &response) {
    // go through service one by one and dispatch
    for (auto &service : services_) {
      bool found = service.get().HandleRequest(ec, url, request, response);
      if (found) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<std::reference_wrapper<Service>> services_;
};

void default_handler(ServiceMiddleware &middl,
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

  std::string request_str = request.get_body_string();
  std::string reply_str;
  std::wstring url = std::wstring(req->CookedUrl.pAbsPath);
  bool found = middl.handle_request(ec, url, request_str, reply_str);
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