#pragma once

#ifdef WINASIO_LOG
#include <boost/log/trivial.hpp>
#endif

// shared by server and client

namespace boost {
namespace wingrpc {

// msg should be moved into moved.
// parse request into proto type
template <typename ProtoIn>
inline void parse_length_prefixed_message(boost::system::error_code &ec,
                                          std::string msg, ProtoIn &proto) {
  if (msg.size() < 5) {
#ifdef WINASIO_LOG
    BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message msg too small";
#endif
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }

  bool compress = msg[0];
  if (compress) {
#ifdef WINASIO_LOG
    BOOST_LOG_TRIVIAL(debug) << "msg compressed";
#endif
  } else {
#ifdef WINASIO_LOG
    BOOST_LOG_TRIVIAL(debug) << "msg content: " << msg;
#endif
  }
  std::uint32_t len = 0;
  len |= (std::uint8_t)msg[4];
  len |= (std::uint8_t)msg[3] << 8;
  len |= (std::uint8_t)msg[2] << 16;
  len |= (std::uint8_t)msg[1] << 24;

  if (msg.size() - 5 != len) {
#ifdef WINASIO_LOG
    BOOST_LOG_TRIVIAL(debug)
        << "parse_length_prefixed_message msg size does not match. encode: "
        << len << "actual: " << (msg.size() - 5);
#endif
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }

#ifdef WINASIO_LOG
  BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message msg debug encode: "
                           << len << "actual: " << (msg.size() - 5);
#endif

  bool ok = proto.ParseFromArray(msg.data() + 5, len);
  if (!ok) {
#ifdef WINASIO_LOG
    BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message bad proto msg";
#endif
    ec = boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
    return;
  }
}

template <typename Proto>
void encode_length_prefixed_message(const Proto &reply, std::string &msgout) {
  // compute msg output size
  std::int32_t len = static_cast<std::int32_t>(reply.ByteSizeLong());
  msgout.resize(len + 5); // length encoded msg size

  // compression byte
  msgout[0] = (char)0;
  // encode 4 byte length in big endian
  int j = 4;
  for (int i = 0; i <= 3; i++) {
    msgout[j] = (len >> (8 * i)) & 0xff;
    j--;
  }
  // add data starting at index 5
  bool ok = reply.SerializeToArray(msgout.data() + 5, len);
  BOOST_ASSERT(ok);
}

} // namespace wingrpc
} // namespace boost