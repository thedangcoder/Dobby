#include "MultiThreadSupport/ThreadSupport.h"

zz::OSThread::LocalStorageKey ThreadSupport::thread_callstack_key_ = 0;

// Get current CallStack
CallStack *ThreadSupport::CurrentThreadCallStack() {

  // TODO: __attribute__((destructor)) is better ?
  if (!thread_callstack_key_) {
    thread_callstack_key_ = zz::OSThread::CreateThreadLocalKey();
  }

  if (zz::OSThread::HasThreadLocal(thread_callstack_key_)) {
    return static_cast<CallStack *>(zz::OSThread::GetThreadLocal(thread_callstack_key_));
  } else {
    CallStack *callstack = new CallStack();
    zz::OSThread::SetThreadLocal(thread_callstack_key_, callstack);
    return callstack;
  }
}
