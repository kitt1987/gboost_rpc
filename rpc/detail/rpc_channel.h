#pragma once
#include "base.h"
#include "ref_count.h"

namespace gboost_rpc {
namespace detail {
class NetworkDispatcher;
class RPCMetaData;
class RPCData;
class RPCChannel : public RefCount {
 public:
  typedef boost::function<void(RPCData*)> RPCDataReceiver;
  typedef boost::function<void()> Cancellation;

  RPCChannel(RPCDataReceiver const& receiver, Cancellation const& cancellation,
             NetworkDispatcher* dispatcher,
             std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
  RPCChannel(RPCDataReceiver&& receiver, Cancellation&& cancellation,
             NetworkDispatcher* dispatcher,
             std::shared_ptr<boost::asio::ip::tcp::socket>& socket);

  virtual ~RPCChannel();

  void TerminateInLoop();

  NetworkDispatcher* GetDispatcher() {
    return dispatcher_;
  }

  void StartToRead() {
    socket_->async_read_some(
        boost::asio::buffer(buffer_),
        boost::bind(&RPCChannel::ReadInLoop, counted_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }

  void SendInLoop(RPCData* data) {
    CHECK(data != NULL);
    pending_.push_back(data);
    WriteInLoop();
  }

 private:
  RPCDataReceiver receiver_;
  Cancellation cancellation_;
  NetworkDispatcher* dispatcher_;
  std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
  RPCData* partial_message_;
  std::vector<int8> buffer_;

  // FIXME Define the pending buffer size
  typedef std::deque<RPCData*> PendingBuffer;
  PendingBuffer pending_;

  bool closed_by_peer_;bool terminated_;

  boost::intrusive_ptr<RPCChannel> counted_this() {
    return boost::intrusive_ptr<RPCChannel>(this);
  }

 protected:
  void ReadInLoop(boost::system::error_code const& error,
                  std::size_t bytes_transferred);
  void WriteInLoop();
  void Written(boost::system::error_code const& error,
               std::size_t bytes_transferred);
  inline void ShutdownInLoop(boost::asio::socket_base::shutdown_type type);
  virtual void RecycleRPCData(RPCData* data) {

  }

  void RebuildSocket(std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
    ClearPendingPackages();
    socket_.swap(socket);
    closed_by_peer_ = false;
  }

  void ClearPendingPackages();bool IsTerminated() {
    return terminated_;
  }

 private:

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RPCChannel);
};
}
}
