#include "rpc_data.h"
#include "rpc_channel.h"
#include "protocol/rpc.pb.h"
#include "util.h"

namespace gboost_rpc {
namespace detail {
RPCData::RPCData()
    : meta_(new RPCMetaData),
      meta_size_(0),
      payload_(NULL),
      is_pending(false) {
  payload_binaries_.reserve(64);
  MEMORY_ALLOCATION_VLOG << "RPCData is allocating, which is " << this;
}

RPCData::RPCData(RPCMetaData* meta, ::google::protobuf::Message const* payload)
    : meta_(meta),
      meta_size_(0),
      payload_(payload),
      is_pending(false) {
  ToBuffer();
}

RPCData::~RPCData() {
  if (!resource_holder_.empty()) {
    resource_holder_();
    MEMORY_ALLOCATION_VLOG << "The resource holder of RPCData " << this
                           << " is deleted";
  }

  char* buffer = boost::asio::buffer_cast<char*>(buffer_);
  if (buffer != NULL)
    delete[] buffer;

  MEMORY_ALLOCATION_VLOG << "RPCData is deleted, which is " << this;
}

void RPCData::DeleteMetaAndPayload(::google::protobuf::Message const* payload) {
  delete payload;
}

static inline void FastAppend(std::vector<int8>* buffer, int8 const* content,
                              uint32 size) {
  std::string::size_type offset = buffer->size();
  buffer->resize(offset + size);
  memcpy(&buffer->at(offset), content, size);
}

int32 RPCData::AppendBinaries(int8 const* binaries, uint32 length) {
  int32 offset = 0;

  if (meta_size_ == 0) {
    CHECK_LT(payload_binaries_.size(), NumMetaSizeBytes);
    uint32 bytes_lacked = NumMetaSizeBytes
        - static_cast<uint32>(payload_binaries_.size());
    if (length < bytes_lacked) {
      FastAppend(&payload_binaries_, binaries, length);
      return length;
    }

    if (!payload_binaries_.empty()) {
      FastAppend(&payload_binaries_, binaries, bytes_lacked);
    }

    int8 const* meta_size_binaries = binaries;
    if (!payload_binaries_.empty()) {
      meta_size_binaries = payload_binaries_.data();
    }

    meta_size_ = ntohl(*(reinterpret_cast<uint32 const*>(meta_size_binaries)));
    CHECK_GT(meta_size_, 0U);
    payload_binaries_.clear();
    offset += bytes_lacked;
    length -= bytes_lacked;
    if (length == 0)
      return offset;
  }

  if (meta_->ByteSize() == 0) {
    CHECK_GT(meta_size_, 0)<< this;
    CHECK_LT(payload_binaries_.size(), meta_size_) << this;

    uint32 bytes_lacked = meta_size_
    - static_cast<uint32>(payload_binaries_.size());
    if (length < bytes_lacked) {
      FastAppend(&payload_binaries_, binaries + offset, length);
      offset += length;
      CHECK_LT(payload_binaries_.size(), meta_size_);
      return offset;
    }

    if (!payload_binaries_.empty()) {
      FastAppend(&payload_binaries_, binaries + offset, bytes_lacked);
      offset += length;
      CHECK_LT(payload_binaries_.size(), meta_size_)<< this;
    }

    int8 const* meta_binaries = binaries + offset;
    if (!payload_binaries_.empty()) {
      meta_binaries = payload_binaries_.data();
    }

    if (!meta_->ParseFromArray(meta_binaries, static_cast<int>(meta_size_))) {
      LOG_ERROR << "Unknown message will be discarded of RPC Channel " << this
      << ", And the channel will be closed";
      return -1;
    }

    NETWORK_BEHAVIOR_VLOG << "Receive " << meta_->DebugString();

    if (meta_->message_length() != 0) {
      payload_binaries_.clear();
      payload_binaries_.reserve(meta_->message_length());
    }

    offset += bytes_lacked;
    length -= bytes_lacked;
    if (length == 0 || meta_->message_length() == 0) {
      return offset;
    }
  }

  CHECK_LT(payload_binaries_.size(), meta_->message_length())<< this;
  uint32 bytes_lacked = meta_->message_length()
      - static_cast<uint32>(payload_binaries_.size());
  if (length < bytes_lacked) {
    FastAppend(&payload_binaries_, binaries + offset, length);
    offset += length;
    CHECK_LT(payload_binaries_.size(), meta_->message_length())<< this;
    return offset;
  }

  FastAppend(&payload_binaries_, binaries + offset, bytes_lacked);
  offset += bytes_lacked;
  CHECK_EQ(payload_binaries_.size(), meta_->message_length());
  return offset;
}

boost::asio::mutable_buffer RPCData::GetBuffer() {
  char* buffer = boost::asio::buffer_cast<char*>(buffer_);
  if (buffer == NULL) {
    ToBuffer();
  }

  is_pending = true;
  return buffer_;
}

void RPCData::ToBuffer() {
  CHECK(meta_ != NULL) << this;
  CHECK(payload_ != NULL) << this;
  int descriptor_size = meta_->ByteSize();
  int message_size = payload_->ByteSize();
  uint32 size = NumMetaSizeBytes + descriptor_size + message_size;
  int8* buffer = NULL;
  char* buffer_reserved = boost::asio::buffer_cast<char*>(buffer_);
  if (buffer_reserved != NULL && boost::asio::buffer_size(buffer_) >= size) {
    buffer = boost::asio::buffer_cast<int8*>(buffer_);
  } else {
    buffer = new int8[size];
  }

  *(reinterpret_cast<int32*>(buffer)) = htonl(descriptor_size);

  // FIXME the AppendToString has bugs which may modify the capacity of std::string.
  //  CHECK(package.AppendToString(&buffer)) << "Bad rpc package";
  //  CHECK(message.AppendToString(&buffer)) << "Bad rpc package";
  void* descriptor_buffer = reinterpret_cast<void*>(buffer + NumMetaSizeBytes);
  if (descriptor_buffer == NULL) {
    LOG_ERROR << "Fail to serialize the RPC message";
    delete[] buffer;
    return;
  }

  if (!meta_->SerializeToArray(descriptor_buffer, descriptor_size)) {
    LOG_ERROR << "Fail to serialize the descriptor of RPC message";
    delete[] buffer;
    return;
  }

  void* message_buffer = reinterpret_cast<void*>(buffer + NumMetaSizeBytes
      + descriptor_size);
  if (message_buffer == NULL) {
    LOG_ERROR << "Fail to serialize the RPC message";
    delete[] buffer;
    return;
  }

  if (!payload_->SerializeToArray(message_buffer, message_size)) {
    LOG_ERROR << "Fail to serialize the RPC message";
    delete[] buffer;
    return;
  }

  buffer_ = boost::asio::mutable_buffer(buffer, size);
}

bool RPCData::Partial() {
  if (meta_->ByteSize() == 0)
    return true;

  uint32 message_length = static_cast<uint32>(payload_binaries_.size());
  CHECK_LE(message_length, meta_->message_length());
  return message_length < meta_->message_length();
}

void RPCData::Clear() {
  meta_size_ = 0;
  is_pending = false;
  payload_binaries_.clear();

  if (meta_)
    meta_->Clear();

  if (!resource_holder_.empty()) {
    resource_holder_();
    resource_holder_.clear();
  }

  MEMORY_ALLOCATION_VLOG << "RPCData " << this << " is clear";
}
}
}
