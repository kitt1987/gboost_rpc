#pragma once
#include "base.h"
#include <atomic>

namespace gboost_rpc {
class AtomicEvent {
 public:
  AtomicEvent()
      : value_(0) {
  }

  ~AtomicEvent() {
  }

  void Notify() {
    std::atomic_fetch_add(&value_, 1);
  }

  void Wait() {
    while (true) {
      int expected = std::atomic_load_explicit(&value_,
                                               std::memory_order_relaxed);
      if (expected > 0) {
        int disired = expected - 1;
        if (std::atomic_compare_exchange_weak(&value_, &expected, disired)) {
          break;
        }
      }
    }
  }

//  bool TimedWait(uint32 duration_in_ms) {
//
//  }

  int GetValue() {
    return std::atomic_load(&value_);
  }

 private:
  std::atomic<int> value_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(AtomicEvent);
};
}
