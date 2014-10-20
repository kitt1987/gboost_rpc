#pragma once
#include "rpc_channel.h"
#include "sem_event.h"

namespace gboost_rpc {
namespace detail {
class SessionManager;
class ClientRPCChannel : public RPCChannel {
 public:
  template<typename ClientRPCDataReceiver>
  ClientRPCChannel(ClientRPCDataReceiver const& receiver,
                   NetworkDispatcher* dispatcher,
                   std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                   SessionManager* session_manager,
                   boost::asio::ip::tcp::resolver::iterator const& server)
      : RPCChannel(boost::bind(receiver, _1, this),
                   boost::bind(&ClientRPCChannel::ReconnectOrFail, this),
                   dispatcher, socket),
        session_manager_(session_manager),
        server_(server) {
  }

  template<typename ClientRPCDataReceiver>
  ClientRPCChannel(ClientRPCDataReceiver&& receiver,
                   NetworkDispatcher* dispatcher,
                   std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                   SessionManager* session_manager,
                   boost::asio::ip::tcp::resolver::iterator const& server)
      : RPCChannel(boost::bind(std::move(receiver), _1, this),
                   boost::bind(&ClientRPCChannel::ReconnectOrFail, this),
                   dispatcher, socket),
        session_manager_(session_manager),
        server_(server) {
  }

  virtual ~ClientRPCChannel();

  void Send(const ::google::protobuf::MethodDescriptor* method,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure* done);

  void SendPendingSessions();

 private:
  SessionManager* session_manager_;
  boost::asio::ip::tcp::resolver::iterator server_;

  void SendInLoop(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done);
  void ReconnectOrFail();
  void Connected(std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
                 boost::system::error_code const& error);

  boost::intrusive_ptr<ClientRPCChannel> counted_this() {
    return boost::intrusive_ptr<ClientRPCChannel>(this);
  }

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(ClientRPCChannel);
};
}
}
