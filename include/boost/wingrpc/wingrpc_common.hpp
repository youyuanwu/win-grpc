#pragma once

#include <boost/log/trivial.hpp>

// shared by server and client

namespace boost
{
    namespace wingrpc
    {

        // msg should be moved into moved.
        // parse request into proto type
        template <typename ProtoIn>
        inline void parse_length_prefixed_message(boost::system::error_code & ec, std::string msg, ProtoIn & proto)
        {
            if (msg.size() < 5)
            {
                BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message msg too small";
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
                return;
            }

            bool compress = msg[0];
            std::size_t len =
                (int) msg[4] | (int) msg[3] << 8 | (int) msg[2] << 16 | (int) msg[1] << 24;
            if (msg.size() - 5 != len)
            {
                BOOST_LOG_TRIVIAL(debug)
                    << "parse_length_prefixed_message msg size does not match";
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
                return;
            }
            std::string msg_body(msg.begin() + 5, msg.end());
            bool ok = proto.ParseFromString(msg_body);
            if (!ok)
            {
                BOOST_LOG_TRIVIAL(debug) << "parse_length_prefixed_message bad proto msg";
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
                return;
            }
        }

        template <typename Proto>
        void encode_length_prefixed_message(const Proto & reply, std::string & msgout)
        {
            std::string encoded_reply = reply.SerializeAsString();
            std::int32_t reply_len = static_cast<std::int32_t>(encoded_reply.size());
            std::vector<BYTE> meta(4, 0); // encode 4 bytes big endian for length
            int j = 3;
            for (int i = 0; i <= 3; i++)
            {
                meta[j] = (reply_len >> (8 * i)) & 0xff;
                j--;
            }
            msgout = std::string(1, (BYTE) 0); // size and char
            msgout += std::string(meta.begin(), meta.end());
            msgout += encoded_reply;
        }

    } // namespace wingrpc
} // namespace boost