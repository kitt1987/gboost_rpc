#include "network_dispatcher.h"
#include "rpc_channel.h"
#include "rpc_data.h"
#include "util.h"

namespace gboost_rpc {
namespace detail {

NetworkDispatcher::NetworkDispatcher(int32 num_channels_threshold)
    : num_channels_threshold_(num_channels_threshold),
      need_stop_(false),
      io_service_(1) {
  CHECK_GE(num_channels_threshold, 0)<< num_channels_threshold;
  MEMORY_ALLOCATION_VLOG << "NetworkDispatcher is allocating, which is " << this;
}

NetworkDispatcher::~NetworkDispatcher() {
  MEMORY_ALLOCATION_VLOG << "NetworkDispatcher is deleted, which is " << this;
}

void NetworkDispatcher::CloseAllChannelsInLoop() {
  for (ChannelPool::iterator each = channel_pool_.begin();
      each != channel_pool_.end(); ++each) {
    each->second->TerminateInLoop();
  }
}

void NetworkDispatcher::ReleaseChannel(RPCChannel* channel) {
  ChannelIterator destination = channel_pool_.find(
      reinterpret_cast<int64>(channel));
  if (destination != channel_pool_.end()) {
    channel_pool_.erase(destination);
    channel->UnRef();
    NETWORK_BEHAVIOR_VLOG << "Channel " << channel
                          << " is removed from dispatcher";
  }
}

bool NetworkDispatcher::SaveChannel(RPCChannel* channel) {
  if (channel_pool_.size() == num_channels_threshold_) {
    LOG_WARNING << "The channel pool is full. New channels will be discarded"
                << ",num_channels_threshold_ = " << num_channels_threshold_;
    LOG_WARNING << "The RPC Channel " << channel << " will be discarded";
    return false;
  }

  CHECK(
      channel_pool_.insert(
          std::make_pair(reinterpret_cast<int64>(channel), channel)).second);
  channel->Ref();
  return true;
}

void NetworkDispatcher::StartChannelInLoop(
    boost::intrusive_ptr<RPCChannel> const& channel) {
  if (!SaveChannel(channel.get())) {
    channel->TerminateInLoop();
    return;
  }

  // CAUTION DO NOT call the Start method in the constructor of RPCChannel
  channel->StartToRead();
}

void NetworkDispatcher::TerminateChannelInLoop(RPCChannel* channel) {
  channel->TerminateInLoop();
}
}
}
