#pragma once
#include "rpc_channel.h"

namespace gboost_rpc {
namespace detail {
class ServerRPCChannel : public RPCChannel {
 public:
  template<typename ServerRPCDataReceiver, typename ServerCancellation>
  ServerRPCChannel(ServerRPCDataReceiver const& receiver,
                   ServerCancellation const& cancellation,
                   NetworkDispatcher* dispatcher,
                   std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
      : RPCChannel(boost::bind(receiver, _1, this),
                   boost::bind(cancellation, this), dispatcher, socket) {
  }

  template<typename ServerRPCDataReceiver, typename ServerCancellation>
  ServerRPCChannel(ServerRPCDataReceiver&& receiver,
                   ServerCancellation&& cancellation,
                   NetworkDispatcher* dispatcher,
                   std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
      : RPCChannel(boost::bind(std::move(receiver), _1, this),
                   boost::bind(std::move(cancellation), this), dispatcher,
                   socket) {
  }

  virtual ~ServerRPCChannel();

 private:
  virtual void RecycleRPCData(RPCData* data);

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(ServerRPCChannel);
};
}
}
