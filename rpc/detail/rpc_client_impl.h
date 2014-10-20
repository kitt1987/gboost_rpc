#pragma once
#include "rpc_client.h"

namespace gboost_rpc {
namespace detail {
class ClientRPCChannel;
class RPCData;
class NetworkDispatcher;
class SessionManager;

class RPCClientImpl : public RPCClient {
 public:
  explicit RPCClientImpl(bool auto_reconnect);
  virtual ~RPCClientImpl();
  virtual bool Connect(std::string const& server, uint32 port, uint32 timeout);
  virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                          ::google::protobuf::RpcController* controller,
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* response,
                          ::google::protobuf::Closure* done);

 private:
  std::unique_ptr<SessionManager> session_manager_;
  std::unique_ptr<NetworkDispatcher> dispatcher_;

  bool auto_reconnect_;
  uint32 timeout_;
  boost::asio::ip::tcp::resolver::iterator server_;

  boost::intrusive_ptr<ClientRPCChannel> connection_;

  void Connected(std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                 boost::asio::deadline_timer* expired_timer,
                 boost::system::error_code const& error);
  void CloseSocket(std::shared_ptr<boost::asio::ip::tcp::socket> const& socket);
  void ReceivePackage(RPCData* package, ClientRPCChannel* channel);
  void ExpireToConnect(
      std::shared_ptr<boost::asio::ip::tcp::socket> const& socket,
      const boost::system::error_code& error);
  void SetupConnectingTimer(
      std::shared_ptr<boost::asio::ip::tcp::socket> const& socket);

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCClientImpl);
};
}
}
