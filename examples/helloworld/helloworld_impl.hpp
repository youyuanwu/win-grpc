#pragma once

#include "helloworld.pb.h"
#include "helloworld.win_grpc.pb.h"

class server2 : public Greeter::Service
{
public:
    void SayHello(boost::system::error_code & ec, const helloworld::HelloRequest * request, helloworld::HelloReply * reply) override
    {
        reply->set_message("hello " + request->name());
    }
};
