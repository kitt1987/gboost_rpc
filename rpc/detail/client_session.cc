#include "client_session.h"
#include "protocol/rpc.pb.h"
#include "helper.h"
#include "util.h"
#include "rpc_data.h"

DEFINE_int32(num_idle_sessions, 64, "Number of cached idle client sessions");

namespace gboost_rpc {
namespace detail {
ClientSession::ClientSession(RPCData* rpc_data,
                             ::google::protobuf::Message* response,
                             ::google::protobuf::Closure* done)
    : data_(rpc_data),
      response_(response),
      done_(done) {
  MEMORY_ALLOCATION_VLOG << "ClientSession is allocating, which is " << this;
}

ClientSession::~ClientSession() {
  MEMORY_ALLOCATION_VLOG << "ClientSession is deleted, which is " << this;
}

void ClientSession::Done(int8 const* data, uint32 length) {
  CHECK(response_->ParseFromArray(data, length));
  done_->Run();
}

bool ClientSession::IsPending() const {
  return data_->IsPending();
}

void ClientSession::ClearPendingState() {
  data_->ClearPendingState();
}

SessionManager::SessionManager()
    : seq_(0) {

}

SessionManager::~SessionManager() {
  ClearHeapElemsInAssociativeContainer(&sessions_);
}

ClientSession* SessionManager::SaveSession(
    const ::google::protobuf::MethodDescriptor* method,
    const ::google::protobuf::Message* request,
    ::google::protobuf::Message* response, ::google::protobuf::Closure* done) {
  RPCMetaData* meta = new RPCMetaData;
  meta->set_seq(seq_);
  meta->set_method_id(method->index());
  meta->set_service_id(GetServiceID(method->service()->full_name()));
  meta->set_message_length(request->ByteSize());
  RPCData* pre_allocated = new RPCData(meta, request);

  ClientSession* session = new ClientSession(pre_allocated, response, done);
  bool result = sessions_.insert(std::make_pair(seq_, session)).second;
  CHECK(result) << seq_;
  NETWORK_BEHAVIOR_VLOG << "Save session of " << seq_;
  ++seq_;
  return session;
}

ClientSession* SessionManager::ReleaseSession(SeqType seq) {
  PackageIterator destination = sessions_.find(seq);
  if (destination == sessions_.end()) {
    LOG_WARNING << "The request is cancelled, which seq is " << seq;
    return NULL;
  }

  ClientSession* session = destination->second;
  sessions_.erase(destination);
  return session;
}
}
}
