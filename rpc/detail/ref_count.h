#pragma once
#include "base.h"

namespace gboost_rpc {
namespace detail {

class RefCount {
 public:
  RefCount()
      : count_(1) {

  }

  virtual ~RefCount() {
  }

  int32 UsedCount() const {
    return count_;
  }

  void Ref() {
    ++count_;
    MEMORY_ALLOCATION_VLOG << "Ref object " << this << ", which count is "
                           << count_;
  }

  void UnRef() {
    CHECK_GE(count_, 1) << " of " << this;
    if (--count_ == 0) {
      MEMORY_ALLOCATION_VLOG << "UnRef and delete object " << this;

      delete this;
      return;
    }

    MEMORY_ALLOCATION_VLOG << "UnRef object " << this << ", which count is "
                           << count_;
  }

 private:
  int32 count_;

//  friend void intrusive_ptr_add_ref(RefCount* object);
//  friend void intrusive_ptr_release(RefCount* object);

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(RefCount);
};

inline void intrusive_ptr_add_ref(RefCount* object) {
  object->Ref();
}

inline void intrusive_ptr_release(RefCount* object) {
  object->UnRef();
}
}
}
