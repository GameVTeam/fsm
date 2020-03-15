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
  RWLock() : status_(0), waiting_readers_(0), waiting_writers_(0) {}
//  RWLock(const RWLock &) = delete;
//  RWLock(RWLock &&) = delete;
//  RWLock &operator=(const RWLock &) = delete;
//  RWLock &operator=(RWLock &&) = delete;

  void RLock() {
	std::unique_lock<std::mutex> lck(mtx_);
	waiting_readers_ += 1;
	read_cv_.wait(lck, [&]() { return waiting_writers_ == 0 && status_ >= 0; });
	waiting_readers_ -= 1;
	status_ += 1;
  }

  void Lock() {
	std::unique_lock<std::mutex> lck(mtx_);
	waiting_writers_ += 1;
	write_cv_.wait(lck, [&]() { return status_ == 0; });
	waiting_writers_ -= 1;
	status_ = -1;
  }

  void Unlock() {
	std::unique_lock<std::mutex> lck(mtx_);
	if (status_ == -1) {
	  status_ = 0;
	} else {
	  status_ -= 1;
	}
	if (waiting_writers_ > 0) {
	  if (status_ == 0) {
		write_cv_.notify_one();
	  }
	} else {
	  read_cv_.notify_all();
	}
  }

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
  explicit RLockGuard(RWLock &lock) : lock_(lock) {
	lock_.RLock();
  }
  ~RLockGuard() {
	lock_.Unlock();
  }
};

class WLockGuard {
 private:
  RWLock &lock_;

 public:
  explicit WLockGuard(RWLock &lock) : lock_(lock) {
	lock_.Lock();
  }
  ~WLockGuard() {
	lock_.Unlock();
  }
};

class LockGuard {
 private:
  std::mutex &mu_;

 public:
  explicit LockGuard(std::mutex &mu) : mu_(mu) {}

  ~LockGuard() { mu_.unlock(); }
};
}
#endif //RWLOCK__RWLOCK_H_