#include "rpc_server_impl.h"
#include "network_dispatcher.h"
#include "server_rpc_channel.h"
#include "rpc_data.h"
#include "protocol/rpc.pb.h"
#include "util.h"
#include "helper.h"
#include "callback.h"

namespace gboost_rpc {
namespace detail {
RPCServerImpl::RPCServerImpl(std::string const& ip, uint32 port,
                             int32 num_threads, int32 size_network_queue,
                             int32 num_channels_threshold)
    : ip_(ip),
      port_(port),
      i_selected_(0) {
  CHECK_GT(num_threads, 0);
  for (int32 i = 0; i < num_threads; ++i) {
    networkers_.push_back(new NetworkDispatcher(num_channels_threshold));
  }

  NetworkDispatcher* main_dispatcher = networkers_[0];
  acceptor_.reset(
      new boost::asio::ip::tcp::acceptor(main_dispatcher->IOService()));
  signals_.reset(new boost::asio::signal_set(main_dispatcher->IOService()));
}

RPCServerImpl::~RPCServerImpl() {
  ClearHeapElemsInSequentialContainer(&networkers_);
}

bool RPCServerImpl::Init() {
  signals_->add(SIGINT);
  signals_->add(SIGTERM);
  signals_->async_wait(boost::bind(&RPCServerImpl::AsyncStop, this));

  boost::asio::ip::address local_address;
  if (!ip_.empty()) {
    boost::system::error_code error;
    local_address = boost::asio::ip::address::from_string(ip_, error);
    if (error) {
      LOG(ERROR)<< "Fail to parse " << ip_ << " caused by "
      << error.message() << ":" << error.value();
      return false;
    }
  }

  boost::asio::ip::tcp::endpoint endpoint(local_address,
                                          static_cast<short>(port_));

  try {
    acceptor_->open(endpoint.protocol());
    acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_->bind(endpoint);
    acceptor_->listen();
    StartToAccept();
    return true;
  } catch (boost::system::system_error& e) {
    LOG(ERROR)<< "Fail to initialize acceptor caused by [" << e.code()
    << "]" << e.what();
    return false;
  }
}

void RPCServerImpl::StartToAccept() {
  NetworkDispatcher* dispatcher = GetIdlestDispatcher();
  new_socket_.reset(new boost::asio::ip::tcp::socket(dispatcher->IOService()));
  acceptor_->async_accept(
      *new_socket_,
      boost::bind(&RPCServerImpl::Accepted, this, dispatcher,
                  boost::asio::placeholders::error));
}

void RPCServerImpl::Accepted(NetworkDispatcher* dispatcher,
                             boost::system::error_code const& error) {
  if (!error) {
    NETWORK_BEHAVIOR_VLOG << "The new client connected, which is "
                          << new_socket_->remote_endpoint();
    dispatcher->StartChannel(
        boost::intrusive_ptr<RPCChannel>(
            new ServerRPCChannel(
                boost::bind(&RPCServerImpl::CallMethod, this, _1, _2),
                boost::bind(&NetworkDispatcher::ReleaseChannel, dispatcher, _1),
                dispatcher, new_socket_),
            false));
  } else {
    if (!acceptor_->is_open())
      return;

    LOG(ERROR)<< "Fail to accept connections caused by ["
    << error.value() << "]" << error.message();
  }

  StartToAccept();
}

void RPCServerImpl::Run() {
  if (networkers_.size() > 1U) {
    for (Networkers::size_type i = 1; i < networkers_.size(); ++i) {
      networkers_[i]->LoopInAnotherThread();
    }
  }

  networkers_[0]->Loop();
}

void RPCServerImpl::RunInAnotherThread() {
  for (auto dispatcher : networkers_) {
    dispatcher->LoopInAnotherThread();
  }
}

void RPCServerImpl::Stop() {
  networkers_[0]->Post(boost::bind(&RPCServerImpl::CloseAcceptor, this));
  for (auto dispatcher : networkers_) {
    dispatcher->Stop();
  }
}

void RPCServerImpl::AsyncStop() {
  networkers_[0]->Post(boost::bind(&RPCServerImpl::CloseAcceptor, this));
  for (auto dispatcher : networkers_) {
    dispatcher->AsyncStop();
  }
}

void RPCServerImpl::AddService(::google::protobuf::Service* service) {
  ServiceId service_id = GetServiceID(service->GetDescriptor()->full_name());

  ServicePoolInsertionResult result = service_pool_.insert(
      std::make_pair(service_id, service));
  if (result.second)
    return;

  if (service == result.first->second)
    LOG_FATAL << "You added a service to the RpcServer many times!";

  LOG_FATAL << "Damn boost hash function hashes 2 string "
            "to just 1 integer, they are "
            << service->GetDescriptor()->full_name() << " and "
            << result.first->second->GetDescriptor()->full_name();
}

::google::protobuf::Service* RPCServerImpl::GetService(ServiceId service_id) {
  ServiceIterator destination = service_pool_.find(service_id);
  if (destination == service_pool_.end())
    return NULL;

  return destination->second;
}

void RPCServerImpl::CallMethod(RPCData* incoming, ServerRPCChannel* channel) {
  RPCMetaData* meta = new RPCMetaData(*incoming->Meta());

  NETWORK_BEHAVIOR_VLOG << "Receive a package of request " << meta->seq();

  ::google::protobuf::Service* service = GetService(meta->service_id());
  ::google::protobuf::ServiceDescriptor const* service_descriptor = service
      ->GetDescriptor();
  if (service_descriptor == NULL) {
    LOG(ERROR)<<"The service the message asked for is unregistered "
    << meta->service_id();
    return;
  }

  ::google::protobuf::MethodDescriptor const* method_descriptor =
      service_descriptor->method(meta->method_id());
  ::google::protobuf::Message* request = service->GetRequestPrototype(
      method_descriptor).New();
  if (!request->ParseFromArray(incoming->MessageBinary(),
                               meta->message_length())) {
    LOG_ERROR << "Fail to parse request "
              << service->GetRequestPrototype(method_descriptor).GetTypeName()
              << ", length:" << meta->message_length();
    delete request;
    return;
  }

  delete incoming;

  ::google::protobuf::Message* response = service->GetResponsePrototype(
      method_descriptor).New();

  service->CallMethod(
      method_descriptor,
      NULL,
      request,
      response,
      NewCallback(
          boost::bind(&RPCServerImpl::SendResponse, this, channel, meta,
                      request, response)));
}

void RPCServerImpl::SendResponse(ServerRPCChannel* channel, RPCMetaData* meta,
                                 ::google::protobuf::Message* request,
                                 ::google::protobuf::Message* response) {
  meta->set_message_length(response->ByteSize());
  RPCData* data = new RPCData(meta, response);
  data->OwnMetaAndPayload();
  channel->SendInLoop(data);
  NETWORK_BEHAVIOR_VLOG << "Send response " << data << ":"
                        << data->Meta()->seq() << " from RPC Channel:"
                        << channel;
  delete request;
}
}

RPCServer* CreateRPCServer(std::string const& ip, uint32 port,
                           int32 num_threads, int32 size_network_queue,
                           int32 num_channels_threshold) {
  detail::RPCServerImpl* server = new detail::RPCServerImpl(
      ip, port, num_threads, size_network_queue, num_channels_threshold);
  if (server->Init()) {
    return server;
  }

  delete server;
  return NULL;
}
}
