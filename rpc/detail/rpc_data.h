#pragma once
#include "base.h"

namespace gboost_rpc {
namespace detail {
class ServerRPCChannel;
class RPCMetaData;
class RPCData {
 public:
  RPCData();
  RPCData(RPCMetaData* meta, ::google::protobuf::Message const* payload);
  virtual ~RPCData();

  // It returns the length of binaries belongs to the message. -1 on failed.
  int32 AppendBinaries(int8 const* binaries, uint32 length);

  bool Partial();

  void OwnMetaAndPayload() {
    CHECK(resource_holder_.empty());
    resource_holder_ = boost::bind(&RPCData::DeleteMetaAndPayload, payload_);
  }

  boost::asio::mutable_buffer GetBuffer();

  bool IsPending() const {
    return is_pending;
  }

  void ClearPendingState() {
    is_pending = false;
  }

  RPCMetaData* Meta() {
    CHECK(meta_ != NULL);
    return meta_.get();
  }

  int8 const* MessageBinary() {
    CHECK(meta_ != NULL);
    return payload_binaries_.data();
  }

  void Clear();

 private:
  static uint32 const NumMetaSizeBytes = sizeof(uint32);

  ZeroArgumentCB resource_holder_;
  std::unique_ptr<RPCMetaData> meta_;

  // For input
  std::vector<int8> payload_binaries_;
  uint32 meta_size_;

  // For output
  ::google::protobuf::Message const* payload_;
  boost::asio::mutable_buffer buffer_;bool is_pending;

  void ToBuffer();

  static void DeleteMetaAndPayload(::google::protobuf::Message const* payload);

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCData);
};

}
}
