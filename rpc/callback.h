#pragma once
#include "base.h"
#include "sync_event.h"

namespace gboost_rpc {
class DLL_EXPORT CancellableWait {
 public:
  CancellableWait(::google::protobuf::Message* request,
                  ::google::protobuf::Message* response)
      : request_(request),
        response_(response) {

  }

  bool TimedWait(int32 ms) {
    return event_.TimedWait(ms);
  }

  void Done() {
    event_.Notify();
  }

 private:
  std::unique_ptr<google::protobuf::Message> request_;
  std::unique_ptr<google::protobuf::Message> response_;
  SyncEvent event_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(CancellableWait);
};
}

namespace google {
namespace protobuf {

namespace internal {
template<typename CallbackType>
class DLL_EXPORT EasyCallback : public ::google::protobuf::Closure {
 public:
  EasyCallback(CallbackType const& cb, bool self_deleting)
      : cb_(cb),
        self_deleting_(self_deleting) {

  }

  EasyCallback(CallbackType&& cb, bool self_deleting)
      : cb_(std::move(cb)),
        self_deleting_(self_deleting) {
  }

  virtual ~EasyCallback() {

  }

  virtual void Run() {
    bool self_deleting = self_deleting_;
    cb_();
    if (self_deleting)
      delete this;
  }

 private:
  CallbackType cb_;

  bool self_deleting_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(EasyCallback);
};

}

template<typename CallbackType>
DLL_EXPORT inline ::google::protobuf::Closure* NewCallback(
    CallbackType const& cb) {
  return new internal::EasyCallback<CallbackType>(cb, true);
}

template<typename CallbackType>
DLL_EXPORT inline ::google::protobuf::Closure* NewCallback(CallbackType&& cb) {
  return new internal::EasyCallback<CallbackType>(std::move(cb), true);
}

template<typename CallbackType>
DLL_EXPORT inline ::google::protobuf::Closure* NewPermanentCallback(
    CallbackType const& cb) {
  return new internal::EasyCallback<CallbackType>(cb, false);
}

template<typename CallbackType>
DLL_EXPORT inline ::google::protobuf::Closure* NewPermanentCallback(
    CallbackType&& cb) {
  return new internal::EasyCallback<CallbackType>(cb, false);
}

}
}
