#pragma once
#include "base.h"
#include <semaphore.h>
#include <errno.h>
#include <string.h>

namespace gboost_rpc {
class Semaphore {
 public:
  Semaphore() {
    CHECK_EQ(sem_init(&semaphore_, 0, 0), 0)<<"Fail to create semaphore due to ["
    << errno << "]" << strerror(errno);
  }

  ~Semaphore() {
    CHECK_EQ(sem_destroy(&semaphore_), 0)<<"Fail to destroy semaphore due to ["
    << errno << "]" << strerror(errno);
  }

  void Notify() {
    CHECK_EQ(sem_post(&semaphore_), 0)<<"Fail to post semaphore due to ["
    << errno << "]" << strerror(errno);
  }

  void Wait() {
    CHECK_EQ(sem_wait(&semaphore_), 0)<<"Fail to call sem_wait due to ["
    << errno << "]" << strerror(errno);
  }

  bool TimedWait(uint32 duration_in_ms) {
    struct timespec duration;
    duration.tv_sec = duration_in_ms / 1000;
    duration.tv_nsec = (duration_in_ms % 1000) * 1000000;
    if (sem_timedwait(&semaphore_, &duration) == 0) {
      return true;
    }

    CHECK_EQ(errno, ETIMEDOUT) << "Fail to call sem_timedwait due to ["
    << errno << "]" << strerror(errno);
    return false;
  }

  int GetValue() {
    int value = 0;
    CHECK_EQ(sem_getvalue(&semaphore_, &value), 0)<<"Fail to get value of semaphore due to ["
    << errno << "]" << strerror(errno);

    return value;
  }

private:
  sem_t semaphore_;

  DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(Semaphore);
};
}
