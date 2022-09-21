#pragma once

#include "boost/wingrpc/wingrpc_common.hpp"

#include <map>

// client utilities for win-grpc

namespace boost {
namespace wingrpc {

namespace winnet = boost::winasio;
namespace net = boost::asio;

template <typename Executor>
void configure_grpc_session(
    winnet::winhttp::basic_winhttp_session_handle<Executor> &h_session,
    _Out_ boost::system::error_code &ec) {
  // set to http2 version
  const DWORD f_enable_HTTP2 = WINHTTP_PROTOCOL_FLAG_HTTP2;
  h_session.set_option(WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                       (PVOID)&f_enable_HTTP2, sizeof(f_enable_HTTP2), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for http2 failed: " << ec;
    return;
  }

  // This is need to receiveing trailer
  const DWORD f_stream_end = 1; // set to true
  h_session.set_option(WINHTTP_OPTION_REQUIRE_STREAM_END, (PVOID)&f_stream_end,
                       sizeof(f_stream_end), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for stream end failed: " << ec;
    return;
  }

  const DWORD f_tls_protocols =
      WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
  h_session.set_option(WINHTTP_OPTION_SECURE_PROTOCOLS, (PVOID)&f_tls_protocols,
                       sizeof(f_tls_protocols), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << L"set_option for tlsProtocols failed: " << ec;
    return;
  }
}

// holds data that persists during async call
//
// path and accept do not need to be persisted.
// but body and response buff needs to be persisted.
// TODO: in winasio make body and response buff separate from payload.
// much of the payload should not be here.
template <typename Executor> class request_holder {
public:
  typedef Executor executor_type;
  request_holder(const executor_type &ex)
      : h_request_(ex), pl_(), body_buff_(), dynamic_body_buff_(body_buff_) {
    // common for all grpc requests
    this->pl_.method = L"POST";
    this->pl_.accept =
        std::make_optional<winnet::winhttp::header::accept_types>(
            {L"application/grpc+proto"});
    this->pl_.secure = true;
  }

  // use std::move
  void set_body(std::string body) { this->pl_.body = std::move(body); }

  // use move?
  void set_path(std::wstring path) { this->pl_.path = std::move(path); }

  // private:
  winnet::winhttp::basic_winhttp_request_asio_handle<executor_type> h_request_;
  winnet::winhttp::payload pl_;

  // buffer for response
  std::vector<BYTE> body_buff_;
  net::dynamic_vector_buffer<BYTE, std::allocator<BYTE>> dynamic_body_buff_;
};

template <typename Response>
void handle_response(
    std::function<void(boost::system::error_code ec, const Response *response)>
        token,
    std::string response_str) {
  boost::system::error_code ec;
  Response response_proto;
  parse_length_prefixed_message(ec, response_str, response_proto);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "fail to parse response " << ec;
    return;
  }
  token(ec, &response_proto);
}

// class trailer_parser{
// };

// TODO: test this
inline void parse_trailers(_In_ std::wstring const &trailer_str,
                           _Out_ std::map<std::wstring, std::wstring> &out) {
  if (trailer_str.size() <= 2) {
    return;
  }
  std::size_t kv_index = 0;

  while (1) {
    if (kv_index >= trailer_str.size()) {
      break;
    }
    std::size_t pos = trailer_str.find(L"\r\n", kv_index);
    if (pos == kv_index) {
      // last empty \r\n
      break;
    }
    bool last = (pos == std::wstring::npos);

    std::wstring kv = trailer_str.substr(kv_index, pos);
    // BOOST_LOG_TRIVIAL(debug) << "kv : " << kv;

    if (kv.size() == 0) {
      break;
    }

    std::size_t colon_space = kv.find(L": ");
    std::wstring key = kv.substr(0, colon_space);
    std::wstring val = kv.substr(colon_space + 2); // to end of str
    out[key] = val;
    // BOOST_LOG_TRIVIAL(debug) << "key [" << key << "] val [" << val <<"]";

    kv_index = pos;
    kv_index += 2; // skip 2 chars
    if (last) {
      break;
    }
  }
}

inline bool get_trailer_val(_In_ std::map<std::wstring, std::wstring> &trailers,
                            _In_ std::wstring const &key,
                            _Out_ std::wstring &val) {
  if (!trailers.contains(key)) {
    return false;
  }
  val = trailers[key];
  return true;
}

inline bool get_grpc_status(_In_ std::map<std::wstring, std::wstring> &trailers,
                            _Out_ std::wstring &code) {
  const std::wstring key = L"grpc-status";
  return get_trailer_val(trailers, key, code);
}

// message is optional.
inline bool
get_grpc_message(_In_ std::map<std::wstring, std::wstring> &trailers,
                 _Out_ std::wstring &message) {
  const std::wstring key = L"grpc-message";
  return get_trailer_val(trailers, key, message);
}

// validate if request is success
template <typename Executor>
void validate_response(
    winnet::winhttp::basic_winhttp_request_handle<Executor> &h,
    _Out_ boost::system::error_code &ec) {
  // print result
  std::wstring headers;
  winnet::winhttp::header::get_all_raw_crlf(h, ec, headers);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "fail to get headers" << ec;
    return;
  }

  std::wstring trailers;
  winnet::winhttp::header::get_trailers(h, ec, trailers);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug) << "fail to get trailers " << ec;
    return;
  }
  // BOOST_LOG_TRIVIAL(debug) << "trailers: [" << trailers <<"]";
  std::map<std::wstring, std::wstring> trailer_map;
  parse_trailers(trailers, trailer_map);
  std::wstring code;
  bool ok = get_grpc_status(trailer_map, code);
  if (!ok) {
    BOOST_LOG_TRIVIAL(debug) << "cannot get status code" << ec;
    ec = boost::system::errc::make_error_code(
        boost::system::errc::no_message_available);
    return;
  }
  if (code == L"0") {
    // success
    return;
  }
  std::wstring message;
  ok = get_grpc_message(trailer_map, message);
  if (ok) {
    BOOST_LOG_TRIVIAL(debug) << "grpc failed with msg:" << message;
  }
}

// helper to launch async request
template <typename Executor, typename Request, typename Response>
void client_exec(
    std::shared_ptr<request_holder<Executor>> req,
    winnet::winhttp::basic_winhttp_connect_handle<Executor> &h_connect,
    Request *request,
    std::function<void(boost::system::error_code ec, const Response *response)>
        token) {

  std::string request_payload;
  encode_length_prefixed_message(*request, request_payload);
  req->set_body(std::move(request_payload));

  winnet::winhttp::async_exec(
      req->pl_, h_connect, req->h_request_, req->dynamic_body_buff_,
      [req, token = std::move(token)](boost::system::error_code ec,
                                      std::size_t) mutable {
        BOOST_LOG_TRIVIAL(debug) << "async_exec handler";
        if (ec) {
          BOOST_LOG_TRIVIAL(debug) << "Hanlder error" << ec;
          token(ec, nullptr);
          return;
        }
        validate_response(req->h_request_, ec);
        if (ec) {
          token(ec, nullptr);
          return;
        }
        std::string resp_body =
            winnet::winhttp::buff_to_string(req->dynamic_body_buff_);
        handle_response<Response>(token, std::move(resp_body));
      });
}

} // namespace wingrpc
} // namespace boost