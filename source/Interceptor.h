#pragma once

#include "dobby/common.h"
#include "dobby/platform_mutex.h"
#include "MemoryAllocator/MemoryAllocator.h"

#include "TrampolineBridge/Trampoline/Trampoline.h"

typedef enum { kFunctionInlineHook, kInstructionInstrument } InterceptRoutingType;

struct InterceptRouting;
struct Interceptor {
  mutable DobbyMutex mutex;
  struct Entry {
    uint32_t id = 0;

    struct {
      bool arm_thumb_mode;
    } features;

    addr_t fake_func_addr;
    dobby_instrument_callback_t pre_handler;
    dobby_instrument_callback_t post_handler;

    addr_t addr;

    MemBlock patched;
    MemBlock relocated;

    InterceptRouting *routing = nullptr;

    uint8_t *origin_code_ = nullptr;

    Entry(addr_t addr) {
      this->addr = addr;
    }

    ~Entry() {
      if (routing) {
        delete routing;
        routing = nullptr;
      }
      // Note: origin_code_ is freed in restore_orig_code() if called,
      // but we should clean up if not restored
      if (origin_code_) {
        operator delete(origin_code_);
        origin_code_ = nullptr;
      }
    }

    void backup_orig_code() {
      __FUNC_CALL_TRACE__();
      auto orig = (uint8_t *)this->addr;
      uint32_t tramp_size = this->patched.size;
      origin_code_ = (uint8_t *)operator new(tramp_size);
      memcpy(origin_code_, orig, tramp_size);
    }

    void restore_orig_code() {
      __FUNC_CALL_TRACE__();
      DobbyCodePatch((void *)patched.addr(), origin_code_, patched.size);
      operator delete(origin_code_);
      origin_code_ = nullptr;
    }

    void feature_set_arm_thumb(bool thumb) {
      features.arm_thumb_mode = thumb;
    }
  };

  stl::vector<Entry *> entries;

  static Interceptor *Shared();

  Entry *find(addr_t addr) {
    DobbyLockGuard lock(mutex);
    return find_unlocked(addr);
  }

  Entry *remove(addr_t addr) {
    DobbyLockGuard lock(mutex);
    return remove_unlocked(addr);
  }

  void add(Entry *entry) {
    DobbyLockGuard lock(mutex);
    entries.push_back(entry);
  }

  const Entry *get(int i) {
    DobbyLockGuard lock(mutex);
    return entries[i];
  }

  int count() const {
    DobbyLockGuard lock(mutex);
    return entries.size();
  }

private:
  // Unlocked versions for internal use when lock is already held
  Entry *find_unlocked(addr_t addr) {
    for (auto *entry : entries) {
      if (entry->patched.addr() == addr) {
        return entry;
      }
    }
    return nullptr;
  }

  Entry *remove_unlocked(addr_t addr) {
    for (auto iter = entries.begin(); iter != entries.end(); iter++) {
      Entry *entry = *iter;
      if (entry->patched.addr() == addr) {
        entries.erase(iter);
        return entry;
      }
    }
    return nullptr;
  }
};

inline static Interceptor gInterceptor;

inline Interceptor *Interceptor::Shared() {
  return &gInterceptor;
}