#pragma once

#include "platform_features.h"

#if defined(BUILDING_KERNEL)
// Kernel mode: no mutex support, operations are assumed to be serialized
struct DobbyMutex {
  void lock() {}
  void unlock() {}
};
#elif defined(_WIN32)
#include <windows.h>
struct DobbyMutex {
  CRITICAL_SECTION cs;
  DobbyMutex() { InitializeCriticalSection(&cs); }
  ~DobbyMutex() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
};
#else
#include <pthread.h>
struct DobbyMutex {
  pthread_mutex_t mtx;
  DobbyMutex() { pthread_mutex_init(&mtx, nullptr); }
  ~DobbyMutex() { pthread_mutex_destroy(&mtx); }
  void lock() { pthread_mutex_lock(&mtx); }
  void unlock() { pthread_mutex_unlock(&mtx); }
};
#endif

// RAII lock guard
struct DobbyLockGuard {
  DobbyMutex &mutex;
  DobbyLockGuard(DobbyMutex &m) : mutex(m) { mutex.lock(); }
  ~DobbyLockGuard() { mutex.unlock(); }
  // Non-copyable
  DobbyLockGuard(const DobbyLockGuard &) = delete;
  DobbyLockGuard &operator=(const DobbyLockGuard &) = delete;
};
