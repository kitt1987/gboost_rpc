#pragma once
#include "base.h"
#include "sync_event.h"
#include "atomic_event.h"
#include "sem_event.h"

namespace gboost_rpc {
namespace detail {
class RPCChannel;
class RPCData;
class NetworkDispatcher {
 public:
  explicit NetworkDispatcher(int32 num_channels_threshold);
  ~NetworkDispatcher();

  void Loop() {
    keep_working_.reset(new boost::asio::io_service::work(io_service_));
    RunOnce();
    stopped_.Notify();
  }

  void RunOnce() {
    if (need_stop_)
      return;

    NETWORK_BEHAVIOR_VLOG << "IO service starting";
    boost::system::error_code error;
    io_service_.run(error);
    if (error) {
      LOG_ERROR << "Fail to run the io service caused by [" << error.value()
                << "]" << error.message();
    }

    io_service_.reset();
    NETWORK_BEHAVIOR_VLOG << "IO service stopped";
  }

  void Stop() {
    need_stop_ = true;
    Post(boost::bind(&NetworkDispatcher::CloseAllChannelsInLoop, this));
    keep_working_.reset();
    stopped_.Wait();
  }

  void AsyncStop() {
    Post(boost::bind(&NetworkDispatcher::CloseAllChannelsInLoop, this));
    keep_working_.reset();
  }

  void LoopInAnotherThread() {
    CHECK(loop_thread_ == NULL);
    loop_thread_.reset(
        new boost::thread(boost::bind(&NetworkDispatcher::Loop, this)));
  }

  void Clear() {
    need_stop_ = false;
  }

  boost::asio::io_service& IOService() {
    return io_service_;
  }

  template<typename ActionType>
  void Post(ActionType const& action) {
    io_service_.dispatch(action);
  }

  template<typename ActionType>
  void Post(ActionType&& action) {
    io_service_.dispatch(std::move(action));
  }

  void ReleaseChannel(RPCChannel* channel);
  void StartChannel(boost::intrusive_ptr<RPCChannel> const& channel) {
    Post(boost::bind(&NetworkDispatcher::StartChannelInLoop, this, channel));
  }

  void TerminateChannel(RPCChannel* channel) {
    Post(
        boost::bind(&NetworkDispatcher::TerminateChannelInLoop, this, channel));
  }

 private:
  uint32 const num_channels_threshold_;

  bool need_stop_;
  Semaphore stopped_;
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> keep_working_;
  std::unique_ptr<boost::thread> loop_thread_;

  typedef std::map<int64, RPCChannel*> ChannelPool;
  typedef ChannelPool::iterator ChannelIterator;
  ChannelPool channel_pool_;
  mutable boost::mutex channel_pool_mutex_;

  inline bool SaveChannel(RPCChannel* channel);
  void CloseAllChannelsInLoop();
  void StartChannelInLoop(boost::intrusive_ptr<RPCChannel> const& channel);
  void TerminateChannelInLoop(RPCChannel* channel);

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(NetworkDispatcher);
};
}
}
