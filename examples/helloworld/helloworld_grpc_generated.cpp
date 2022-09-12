#include "boost/winasio/http/http.hpp"

#include "helloworld.pb.h"
#include "helloworld_grpc_generated.hpp"

void Ihelloword_handler::SayHello(boost::system::error_code &ec,
                                  const helloworld::HelloRequest &request,
                                  helloworld::HelloReply &reply) {
  ec = boost::system::errc::make_error_code(boost::system::errc::not_supported);
}