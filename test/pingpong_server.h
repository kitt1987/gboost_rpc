#pragma once
#include "base.h"
#include "test/protocol/pingpong.pb.h"

class PingPongServer : public PingPongTesting {
 public:
  PingPongServer();
  virtual ~PingPongServer();

  virtual void PingPong(::google::protobuf::RpcController* controller,
                        const ::Ball* request, ::Ball* response,
                        ::google::protobuf::Closure* done);

 private:

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(PingPongServer);
};
