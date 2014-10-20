#include "rpc_channel.h"
#include "rpc_data.h"
#include "protocol/rpc.pb.h"
#include "network_dispatcher.h"
#include "util.h"
#include "helper.h"

DEFINE_int32(read_buf_size, 8192, "Size of read buffer in each channel");

namespace gboost_rpc {
namespace detail {
RPCChannel::RPCChannel(RPCDataReceiver const& receiver,
                       Cancellation const& cancellation,
                       NetworkDispatcher* dispatcher,
                       std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
    : receiver_(receiver),
      cancellation_(cancellation),
      dispatcher_(dispatcher),
      partial_message_(NULL),
      buffer_(FLAGS_read_buf_size),
      closed_by_peer_(false),
      terminated_(false) {
  socket_.swap(socket);
  MEMORY_ALLOCATION_VLOG << "RPCChannel is allocating, which is " << this;
}

RPCChannel::RPCChannel(RPCDataReceiver&& receiver, Cancellation&& cancellation,
                       NetworkDispatcher* dispatcher,
                       std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
    : receiver_(std::move(receiver)),
      cancellation_(std::move(cancellation)),
      dispatcher_(dispatcher),
      partial_message_(NULL),
      buffer_(FLAGS_read_buf_size),
      closed_by_peer_(false),
      terminated_(false) {
  socket_.swap(socket);
  MEMORY_ALLOCATION_VLOG
      << "RPCChannel[Move constructor] is allocating, which is " << this;
}

RPCChannel::~RPCChannel() {
  CloseSocket(socket_.get());
  ClearPendingPackages();
  MEMORY_ALLOCATION_VLOG << "RPCChannel is deleted, which is " << this;
}

void RPCChannel::ReadInLoop(boost::system::error_code const& error,
                            std::size_t bytes_transferred) {
  if (terminated_) {
    NETWORK_BEHAVIOR_VLOG
        << "The channel " << this
        << " is terminated. All operations of it will be cancelled";
    return;
  }

  if (error) {
    if (error.value() != boost::system::errc::no_such_file_or_directory) {
      LOG_ERROR << "Fail to read data from connection caused by ["
                << error.value() << "]" << error.message();
    }

    if (!closed_by_peer_) {
      closed_by_peer_ = true;
      cancellation_();
    }

    return;
  }

  int32 offset = 0;
  while (bytes_transferred != 0) {
    if (partial_message_ == NULL) {
      partial_message_ = new RPCData;
    }

    int32 bytes_handled = partial_message_->AppendBinaries(
        buffer_.data() + offset, static_cast<uint32>(bytes_transferred));
    if (bytes_handled < 0) {
      LOG_ERROR << "Data received is unformatted";
      delete partial_message_;
      partial_message_ = NULL;
      break;
    }

    CHECK_LE(bytes_handled, bytes_transferred);
    bytes_transferred -= bytes_handled;
    offset += bytes_handled;

    if (partial_message_->Partial()) {
      CHECK_EQ(bytes_transferred, 0);
      continue;
    }

    receiver_(partial_message_);
    partial_message_ = NULL;
  }

  socket_->async_read_some(
      boost::asio::buffer(buffer_),
      boost::bind(&RPCChannel::ReadInLoop, counted_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void RPCChannel::WriteInLoop() {
  if (pending_.empty()) {
    NETWORK_BEHAVIOR_VLOG << "Nothing is pending, all data is sent out";
    return;
  }

  RPCData* first_data = pending_[0];
  if (first_data->IsPending()) {
    NETWORK_BEHAVIOR_VLOG << "Some data is in pending, wait for sending";
    return;
  }

  NETWORK_BEHAVIOR_VLOG << "Send buffer of " << first_data->Meta()->seq()
                        << " to kernel";

  boost::asio::mutable_buffer data_buffer = first_data->GetBuffer();
  CHECK_GT(boost::asio::buffer_size(data_buffer), 0)<< first_data->Meta()->DebugString();
  boost::asio::async_write(
      *socket_,
      boost::asio::buffer(data_buffer),
      boost::bind(&RPCChannel::Written, counted_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void RPCChannel::Written(boost::system::error_code const& error,
                         std::size_t bytes_transferred) {
  if (error) {
    LOG_ERROR << "Fail to write data to ServerRPCChannel " << this
              << " caused by [" << error.value() << "]" << error.message();
    return;
  }

  CHECK_GT(bytes_transferred, 0);

  if (pending_.empty()) {
    NETWORK_BEHAVIOR_VLOG
        << "No pending data is waiting for response, may be cleared due to errors";
    return;
  }

  RPCData* first_data = pending_[0];
  CHECK(first_data->IsPending()) << first_data;
  CHECK_EQ(bytes_transferred, boost::asio::buffer_size(first_data->GetBuffer()))<< bytes_transferred;
  NETWORK_BEHAVIOR_VLOG << first_data->Meta()->seq() << " is sent out";

  pending_.pop_front();
  RecycleRPCData(first_data);
  WriteInLoop();
}

void RPCChannel::ShutdownInLoop(boost::asio::socket_base::shutdown_type type) {
  boost::system::error_code error;
  socket_->shutdown(type, error);
  if (error) {
    LOG_ERROR << "Fail to shutdown channel due to [" << error.value() << "]"
              << error.message();
  }

  NETWORK_BEHAVIOR_VLOG << "The channel will be shutdown, which is " << this;
}

void RPCChannel::TerminateInLoop() {
  CHECK(socket_ != NULL);
  terminated_ = true;
  ShutdownInLoop(boost::asio::ip::tcp::socket::shutdown_both);
  NETWORK_BEHAVIOR_VLOG << "The channel will be terminated, which is " << this;
}

void RPCChannel::ClearPendingPackages() {
  for (auto pending_data : pending_) {
    RecycleRPCData(pending_data);
  }

  pending_.clear();
}
}
}
