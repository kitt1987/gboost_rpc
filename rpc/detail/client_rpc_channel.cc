#include "client_rpc_channel.h"
#include "network_dispatcher.h"
#include "rpc_data.h"
#include "client_session.h"
#include "protocol/rpc.pb.h"
#include "helper.h"

namespace gboost_rpc {
namespace detail {
ClientRPCChannel::~ClientRPCChannel() {

}

void ClientRPCChannel::Send(const ::google::protobuf::MethodDescriptor* method,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done) {
  GetDispatcher()->Post(
      boost::bind(&ClientRPCChannel::SendInLoop, this, method, request,
                  response, done));
}

void ClientRPCChannel::SendInLoop(
    const ::google::protobuf::MethodDescriptor* method,
    const ::google::protobuf::Message* request,
    ::google::protobuf::Message* response, ::google::protobuf::Closure* done) {
  NETWORK_BEHAVIOR_VLOG << "Client sends request to loop:"
                        << request->DebugString();
  ClientSession* session = session_manager_->SaveSession(method, request,
                                                         response, done);
  RPCChannel::SendInLoop(session->Data());
}

void ClientRPCChannel::SendPendingSessions() {
  SessionManager::SessionPool const& sessions =
      session_manager_->GetAllSessions();
  NETWORK_BEHAVIOR_VLOG << "Send " << sessions.size() << " pending packages";
  for (SessionManager::SessionPool::value_type const& session : sessions) {
    session.second->ClearPendingState();
    NETWORK_BEHAVIOR_VLOG << "Send pending data " << session.first << ":"
                          << session.second << " via channel " << this;
    RPCChannel::SendInLoop(session.second->Data());
  }
}

void ClientRPCChannel::ReconnectOrFail() {
  if (server_ == boost::asio::ip::tcp::resolver::iterator())
    return;

  if (IsTerminated())
    return;

  LOG_WARNING << "The client will connect to the server due to errors of "
              << "the last channel " << this;

  auto socket = std::make_shared<boost::asio::ip::tcp::socket>(
      GetDispatcher()->IOService());
  boost::asio::async_connect(
      *socket,
      server_,
      boost::bind(&ClientRPCChannel::Connected, counted_this(), socket,
                  boost::asio::placeholders::error));
}

void ClientRPCChannel::Connected(
    std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
    boost::system::error_code const& error) {
  if (IsTerminated()) {
    CloseSocket(socket.get());
    return;
  }

  if (error) {
    LOG_FIRST_N(ERROR, 2) << "Fail to connect to the server due to " << "["
                          << error.value() << "]" << error.message();

    boost::asio::async_connect(
        *socket,
        server_,
        boost::bind(&ClientRPCChannel::Connected, counted_this(), socket,
                    boost::asio::placeholders::error));
    return;
  }

  NETWORK_BEHAVIOR_VLOG << "Reconnected! The channel is " << this;

  RebuildSocket(socket);
  StartToRead();
  SendPendingSessions();
}
}
}
