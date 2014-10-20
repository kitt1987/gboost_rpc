#pragma once
#include "base.h"

namespace google {
namespace protobuf {
class Message;
}
}

namespace gboost_rpc {
namespace detail {
class RPCMetaData;
class RPCData;
class ClientSession {
 public:
  ClientSession(RPCData* rpc_data, ::google::protobuf::Message* response,
                ::google::protobuf::Closure* done);
  ~ClientSession();

  void Done(int8 const* data, uint32 length);

  RPCData* Data() {
    return data_.get();
  }

  RPCData* ReleaseData() {
    CHECK(IsPending());
    return data_.release();
  }

  bool IsPending() const;
  void ClearPendingState();

 private:
  std::unique_ptr<RPCData> data_;
  ::google::protobuf::Message* response_;
  ::google::protobuf::Closure* done_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN (ClientSession);
};

class SessionManager {
 public:
  typedef std::map<SeqType, ClientSession*> SessionPool;

  SessionManager();
  ~SessionManager();

  bool Empty() const {
    return sessions_.empty();
  }

  SessionPool const& GetAllSessions() const {
    return sessions_;
  }

  ClientSession* SaveSession(const ::google::protobuf::MethodDescriptor* method,
                             const ::google::protobuf::Message* request,
                             ::google::protobuf::Message* response,
                             ::google::protobuf::Closure* done);
  ClientSession* ReleaseSession(SeqType seq);

 private:
  SeqType seq_;
  typedef SessionPool::iterator PackageIterator;
  SessionPool sessions_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN (SessionManager);
};
}
}
