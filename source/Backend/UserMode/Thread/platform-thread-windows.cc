#include "PlatformThread.h"

#include <windows.h>

using namespace zz;

int OSThread::GetCurrentProcessId() {
  return (int)::GetCurrentProcessId();
}

int OSThread::GetCurrentThreadId() {
  return (int)::GetCurrentThreadId();
}

OSThread::LocalStorageKey OSThread::CreateThreadLocalKey() {
  DWORD key = TlsAlloc();
  if (key == TLS_OUT_OF_INDEXES) {
    return 0;
  }
  return (LocalStorageKey)(key + 1); // +1 to distinguish from invalid 0
}

void OSThread::DeleteThreadLocalKey(LocalStorageKey key) {
  if (key > 0) {
    TlsFree((DWORD)(key - 1));
  }
}

void *OSThread::GetThreadLocal(LocalStorageKey key) {
  if (key <= 0) {
    return NULL;
  }
  return TlsGetValue((DWORD)(key - 1));
}

int OSThread::GetThreadLocalInt(LocalStorageKey key) {
  return (int)(intptr_t)GetThreadLocal(key);
}

void OSThread::SetThreadLocal(LocalStorageKey key, void *value) {
  if (key > 0) {
    TlsSetValue((DWORD)(key - 1), value);
  }
}

void OSThread::SetThreadLocalInt(LocalStorageKey key, int value) {
  SetThreadLocal(key, (void *)(intptr_t)value);
}

bool OSThread::HasThreadLocal(LocalStorageKey key) {
  if (key <= 0) {
    return false;
  }
  // On Windows, TlsGetValue returns NULL for unset keys and on error
  // We need to check GetLastError() to distinguish
  SetLastError(ERROR_SUCCESS);
  void *value = TlsGetValue((DWORD)(key - 1));
  return value != NULL || GetLastError() == ERROR_SUCCESS;
}

void *OSThread::GetExistingThreadLocal(LocalStorageKey key) {
  return GetThreadLocal(key);
}
