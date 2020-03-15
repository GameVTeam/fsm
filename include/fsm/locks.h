//
// Created by 方泓睿 on 2020/3/15.
//

#ifndef RWLOCK__RWLOCK_H_
#define RWLOCK__RWLOCK_H_

#include <mutex>
#include <condition_variable>

namespace fsm {
class RWLock {
 public:
  RWLock();
  RWLock(const RWLock &) = delete;
  RWLock(RWLock &&) = delete;
  RWLock &operator=(const RWLock &) = delete;
  RWLock &operator=(RWLock &&) = delete;

  void RLock();

  void Lock();

  void Unlock();

 private:
  // -1    : one writer
  // 0     : no reader and no writer
  // n > 0 : n reader
  int32_t status_;
  int32_t waiting_readers_;
  int32_t waiting_writers_;
  std::mutex mtx_;
  std::condition_variable read_cv_;
  std::condition_variable write_cv_;
};

class RLockGuard {
 private:
  RWLock &lock_;

 public:
  explicit RLockGuard(RWLock &lock);
  ~RLockGuard();
};

class WLockGuard {
 private:
  RWLock &lock_;

 public:
  explicit WLockGuard(RWLock &lock);
  ~WLockGuard();
};

class LockGuard {
 private:
  std::mutex &mu_;

 public:
  explicit LockGuard(std::mutex &mu);

  ~LockGuard();
};
}
#endif //RWLOCK__RWLOCK_H_