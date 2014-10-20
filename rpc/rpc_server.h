#pragma once
#include "base.h"

namespace google {
namespace protobuf {
class Service;
}
}

namespace gboost_rpc {

class DLL_EXPORT RPCServer {
 public:
  RPCServer();
  virtual ~RPCServer();
  virtual void Run() = 0;
  virtual void RunInAnotherThread() = 0;
  virtual void Stop() = 0;
  virtual void AddService(::google::protobuf::Service* service) = 0;

 private:

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCServer);
};

DLL_EXPORT RPCServer* CreateRPCServer(std::string const& ip, uint32 port,
                           int32 num_threads, int32 size_network_queue,
                           int32 num_allow_connect);
}
