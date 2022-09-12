#pragma once

#include "helloworld.pb.h"
#include "helloworld_grpc_generated.hpp"

class server : public Ihelloword_handler {
public:
  void SayHello(boost::system::error_code &ec,
                const helloworld::HelloRequest &request,
                helloworld::HelloReply &reply) override {
    reply.set_message("hello " + request.name());
  }
};
