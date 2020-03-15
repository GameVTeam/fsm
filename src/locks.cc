//
// Created by 31838 on 3/15/2020.
//

#include "fsm/locks.h"

namespace fsm {
RWLock::RWLock() : status_(0), waiting_readers_(0), waiting_writers_(0) {}

void RWLock::RLock() {
  std::unique_lock<std::mutex> lck(mtx_);
  waiting_readers_ += 1;
  read_cv_.wait(lck, [&]() { return waiting_writers_ == 0 && status_ >= 0; });
  waiting_readers_ -= 1;
  status_ += 1;
}

void RWLock::Lock() {
  std::unique_lock<std::mutex> lck(mtx_);
  waiting_writers_ += 1;
  write_cv_.wait(lck, [&]() { return status_ == 0; });
  waiting_writers_ -= 1;
  status_ = -1;
}

void RWLock::Unlock() {
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

RLockGuard::~RLockGuard() {
  lock_.Unlock();
}

RLockGuard::RLockGuard(RWLock &lock) : lock_(lock) {
  lock_.RLock();
}

WLockGuard::WLockGuard(RWLock &lock) : lock_(lock) {
  lock_.Lock();
}

WLockGuard::~WLockGuard() {
  lock_.Unlock();
}

LockGuard::LockGuard(std::mutex &mu) : mu_(mu) {
  mu_.lock();
}

LockGuard::~LockGuard() {
  mu_.unlock();
}
}