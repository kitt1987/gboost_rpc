#include "server_rpc_channel.h"
#include "network_dispatcher.h"
#include "rpc_data.h"

namespace gboost_rpc {
namespace detail {
ServerRPCChannel::~ServerRPCChannel() {
}

void ServerRPCChannel::RecycleRPCData(RPCData* data) {
  delete data;
}
}
}
