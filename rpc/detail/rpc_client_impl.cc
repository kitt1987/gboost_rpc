#include "rpc_client_impl.h"
#include "network_dispatcher.h"
#include "client_rpc_channel.h"
#include "client_session.h"
#include "rpc_data.h"
#include "protocol/rpc.pb.h"
#include "util.h"
#include <boost/intrusive_ptr.hpp>

namespace gboost_rpc {
namespace detail {
RPCClientImpl::RPCClientImpl(bool auto_reconnect)
    : session_manager_(new SessionManager),
      dispatcher_(new NetworkDispatcher(0)),
      auto_reconnect_(auto_reconnect),
      timeout_(0) {
}

RPCClientImpl::~RPCClientImpl() {
  dispatcher_->TerminateChannel(connection_.get());
  dispatcher_->Stop();
}

bool RPCClientImpl::Connect(std::string const& server, uint32 port,
                            uint32 timeout) {
  if (connection_ != NULL)
    return false;

  timeout_ = timeout;
  boost::system::error_code parsing_error;
  boost::asio::ip::address local_address =
      boost::asio::ip::address::from_string(server, parsing_error);
  if (parsing_error) {
    LOG_ERROR << "Fail to parse " << server << " due to ["
              << parsing_error.value() << "]" << parsing_error.message();
    return false;
  }

  server_ = boost::asio::ip::tcp::resolver::iterator::create(
      boost::asio::ip::tcp::endpoint(local_address,
                                     static_cast<unsigned short>(port)),
      "", "");
  boost::asio::ip::tcp::resolver::iterator invalid;
  CHECK(server_ != invalid) << server << "," << port;

  auto socket = std::make_shared<boost::asio::ip::tcp::socket>(
      dispatcher_->IOService());

  do {
    boost::asio::deadline_timer* timer = NULL;
    if (auto_reconnect_) {
      boost::asio::deadline_timer connecting_timer(dispatcher_->IOService());
      connecting_timer.expires_from_now(
          boost::posix_time::milliseconds(timeout_));
      connecting_timer.async_wait(
          boost::bind(&RPCClientImpl::ExpireToConnect, this, socket,
                      boost::asio::placeholders::error));
      timer = &connecting_timer;
      SetupConnectingTimer(socket);
    }

    boost::asio::async_connect(
        *socket,
        server_,
        boost::bind(&RPCClientImpl::Connected, this, socket, timer,
                    boost::asio::placeholders::error));
    dispatcher_->RunOnce();

    if (connection_ != NULL) {
      NETWORK_BEHAVIOR_VLOG << "Connected, the new connection is "
                            << connection_.get() << " with socket "
                            << socket.get();
      connection_->StartToRead();
      dispatcher_->LoopInAnotherThread();
      break;
    }

    NETWORK_BEHAVIOR_VLOG << "Fail to connect via socket " << socket.get();

    if (!auto_reconnect_)
      break;
  } while (true);

  return connection_ != NULL;
}

void RPCClientImpl::ExpireToConnect(
    std::shared_ptr<boost::asio::ip::tcp::socket> const& socket,
    const boost::system::error_code& error) {
  if (error) {
    if (error.value() == boost::system::errc::operation_canceled)
      return;

    LOG_ERROR << "Connecting Timer failed due to [" << error.value() << "]"
              << error.message();
  }

  NETWORK_BEHAVIOR_VLOG << "Expire to connect via the socket " << socket.get();
  CloseSocket(socket);
}

void RPCClientImpl::SetupConnectingTimer(
    std::shared_ptr<boost::asio::ip::tcp::socket> const& socket) {
  boost::asio::deadline_timer connecting_timer(dispatcher_->IOService());
  connecting_timer.expires_from_now(boost::posix_time::milliseconds(timeout_));
  connecting_timer.async_wait(
      boost::bind(&RPCClientImpl::ExpireToConnect, this, socket,
                  boost::asio::placeholders::error));
}

void RPCClientImpl::Connected(
    std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
    boost::asio::deadline_timer* expiring_timer,
    boost::system::error_code const& error) {
  if (error) {
    LOG_FIRST_N(ERROR, 2)
    << "Fail to connect to " << server_->host_name() << " due to " << "["
    << error.value() << "]" << error.message();
  } else {
    connection_ = boost::intrusive_ptr<ClientRPCChannel>(
        new ClientRPCChannel(
            boost::bind(&RPCClientImpl::ReceivePackage, this, _1, _2),
            dispatcher_.get(),
            socket,
            session_manager_.get(),
            auto_reconnect_ ?
                server_ : boost::asio::ip::tcp::resolver::iterator()),
        false);
  }

  if (expiring_timer != NULL) {
    boost::system::error_code error_code;
    expiring_timer->cancel(error_code);
    if (error_code) {
      LOG_ERROR << "Fail to cancel the expiring timer due to ["
                << error_code.value() << "]" << error_code.message();
    }
  }
}

void RPCClientImpl::CloseSocket(
    std::shared_ptr<boost::asio::ip::tcp::socket> const& socket) {
  boost::system::error_code error;
  socket->close(error);
  if (error) {
    LOG_ERROR << "Fail to close the socket due to [" << error.value() << "]"
              << error.message();
  }

  NETWORK_BEHAVIOR_VLOG << "The socket " << socket.get() << " will be closed";
}

void RPCClientImpl::CallMethod(
    const ::google::protobuf::MethodDescriptor* method,
    ::google::protobuf::RpcController* controller,
    const ::google::protobuf::Message* request,
    ::google::protobuf::Message* response, ::google::protobuf::Closure* done) {
  CHECK(connection_ != NULL);
  connection_->Send(method, request, response, done);
}

// CAUTION This method will be called in the network thread or dispatcher thread
void RPCClientImpl::ReceivePackage(RPCData* incoming,
                                   ClientRPCChannel* channel) {
  CHECK(incoming != NULL);
  RPCMetaData* meta = incoming->Meta();
  NETWORK_BEHAVIOR_VLOG << "A package is received, which seq is "
                        << meta->seq();

  ClientSession* session = session_manager_->ReleaseSession(meta->seq());
  if (session != NULL) {
    NETWORK_BEHAVIOR_VLOG << "A session is completed, which seq is "
                          << meta->seq();
    session->Done(incoming->MessageBinary(), meta->message_length());
    RPCData* data = session->ReleaseData();
    delete data;
    delete session;
  }

  delete incoming;
}
}

RPCClient* CreateRPCClient(std::string const& server, uint32 port,
                           uint32 timeout, bool auto_reconnect) {
  RPCClient* client = new detail::RPCClientImpl(auto_reconnect);
  if (client->Connect(server, port, timeout))
    return client;

  delete client;
  return NULL;
}
}
