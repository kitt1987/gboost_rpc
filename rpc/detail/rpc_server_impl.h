#pragma once
#include "rpc_server.h"

namespace gboost_rpc {
namespace detail {
class RPCMetaData;
class RPCData;
class ServerRPCChannel;
class NetworkDispatcher;

class RPCServerImpl : public RPCServer {
 public:
  RPCServerImpl(std::string const& ip, uint32 port, int32 num_threads,
                int32 size_network_queue, int32 num_channels_threshold);
  virtual ~RPCServerImpl();
  virtual void Run();
  virtual void RunInAnotherThread();
  virtual void Stop();
  virtual void AddService(::google::protobuf::Service* service);

  bool Init();

 private:
  std::string ip_;
  uint32 port_;

  typedef std::map<ServiceId, ::google::protobuf::Service*> ServicePool;
  typedef ServicePool::iterator ServiceIterator;
  typedef std::pair<ServiceIterator, bool> ServicePoolInsertionResult;
  ServicePool service_pool_;

  typedef std::vector<NetworkDispatcher*> Networkers;
  Networkers networkers_;

  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  std::unique_ptr<boost::asio::signal_set> signals_;
  std::shared_ptr<boost::asio::ip::tcp::socket> new_socket_;

  inline ::google::protobuf::Service* GetService(ServiceId service_id);
  void CallMethod(RPCData* package, ServerRPCChannel* channel);
  void SendResponse(ServerRPCChannel* channel, RPCMetaData* meta,
                    ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response);

  void Accepted(NetworkDispatcher* dispatcher,
                boost::system::error_code const& error);
  void StartToAccept();
  void CloseAcceptor() {
    boost::system::error_code error;
    acceptor_->close(error);
    if (error) {
      LOG(ERROR)<<"Fail to close acceptor due to " << error.value() << ":"
      << error.message();
    }
  }

  void AsyncStop();

  Networkers::size_type i_selected_;
  NetworkDispatcher* GetIdlestDispatcher() {
    CHECK_LT(i_selected_, networkers_.size());
    NetworkDispatcher* selected = networkers_[i_selected_];
    ++i_selected_;
    i_selected_ %= networkers_.size();
    return selected;
  }

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCServerImpl);
};
}
}
