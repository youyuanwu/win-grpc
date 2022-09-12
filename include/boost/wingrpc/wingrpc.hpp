#pragma once

#include <any>
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
    std::function<void(boost::system::error_code &ec, const Request &, Reply &)>
        op) {
  Request req;
  parse_length_prefixed_message(ec, request_str, req);

  if (ec) {
    // response.add_trailer("grpc-status", "3"); // invalid argument
    // response.add_trailer("grpc-message", "invalid-body");
    return;
  }

  Reply reply;
  op(ec, req, reply);

  if (ec) {
    // TODO
    return;
  }

  encode_length_prefixed_message(reply, reply_str);
}

class router {
public:
  template <typename Request, typename Reply>
  void add_operation(std::wstring url,
                     std::function<void(boost::system::error_code &ec,
                                        const Request &, Reply &)>
                         op) {
    this->add_route(url, op);
  }

  // returns the std::function
  template <typename Request, typename Reply>
  auto resolve_operation(boost::system::error_code &ec, std::wstring url) {
    std::any any_op;
    this->resolve(ec, any_op, url);

    using operation_type = std::function<void(boost::system::error_code & ec,
                                              const Request &, Reply &)>;

    if (ec) {
      BOOST_LOG_TRIVIAL(debug) << "fail to resolve";
      return operation_type();
    }
    auto fn = std::any_cast<operation_type>(any_op);
    return fn;
  }

  template <typename Request, typename Reply>
  void dispatch(boost::system::error_code &ec, std::wstring url,
                const std::string request_str, std::string &reply_str) {
    auto fn = this->resolve_operation<Request, Reply>(ec, url);
    if (ec) {
      return;
    }

    handle_request<Request, Reply>(ec, request_str, reply_str, fn);
  }

private:
  void resolve(boost::system::error_code &ec, std::any &op, std::wstring url) {
    if (!this->routes_.contains(url)) {
      ec =
          boost::system::errc::make_error_code(boost::system::errc::no_message);
      return;
    }
    op = this->routes_[url];
  }

  void add_route(std::wstring url, std::any operation) {
    this->routes_[url] = operation;
  }

  std::map<std::wstring, std::any> routes_;
};

} // namespace wingrpc
} // namespace boost