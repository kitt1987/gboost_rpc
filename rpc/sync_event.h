#pragma once
#include "base.h"

namespace gboost_rpc {
class DLL_EXPORT SyncEvent {
 public:
  explicit SyncEvent(bool auto_reset = false)
      : value_(0),
        reset_(auto_reset) {
  }

  ~SyncEvent() {
  }

  void Wait() {
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (value_ == 0) {
      signal_.wait(lock);
    }

    CHECK_GT(value_, 0)<< "Damn condition variable";
    --value_;
    if (reset_)
      value_ = 0;
  }

  bool TimedWait(uint32 duration_in_ms) {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (value_ == 0) {
      return signal_.timed_wait(lock,
                                boost::posix_time::milliseconds(duration_in_ms));
    }

    CHECK_GT(value_, 0)<< "Damn condition variable";
    --value_;

    if (reset_)
      value_ = 0;

    return true;
  }

  void Notify() {
    boost::lock_guard<boost::mutex> lock(mutex_);
    ++value_;
    signal_.notify_all();
  }

 private:
  boost::mutex mutex_;
  boost::condition_variable signal_;

  int32 value_;

  bool reset_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(SyncEvent);
};
}
