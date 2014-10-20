#pragma once
#include "base.h"

namespace gboost_rpc {
class DLL_EXPORT RPCClient : public ::google::protobuf::RpcChannel {
 public:
  RPCClient();
  virtual ~RPCClient();
  virtual bool Connect(std::string const& server, uint32 port,
                       uint32 timeout) = 0;

 private:
  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCClient);
};

DLL_EXPORT RPCClient* CreateRPCClient(std::string const& server, uint32 port,
                           uint32 timeout, bool auto_reconnect);
}
