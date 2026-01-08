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

  // Use hash map for O(1) lookup instead of O(n) linear search
  stl::unordered_map<addr_t, Entry *> entries_map;

  static Interceptor *Shared();

  // O(1) lookup by address
  Entry *find(addr_t addr) {
    DobbyLockGuard lock(mutex);
    auto it = entries_map.find(addr);
    if (it != entries_map.end()) {
      return it->second;
    }
    return nullptr;
  }

  // O(1) removal by address
  Entry *remove(addr_t addr) {
    DobbyLockGuard lock(mutex);
    auto it = entries_map.find(addr);
    if (it != entries_map.end()) {
      Entry *entry = it->second;
      entries_map.erase(it);
      return entry;
    }
    return nullptr;
  }

  // O(1) insertion
  void add(Entry *entry) {
    DobbyLockGuard lock(mutex);
    entries_map.insert(stl::pair<addr_t, Entry *>(entry->addr, entry));
  }

  int count() const {
    DobbyLockGuard lock(mutex);
    return entries_map.size();
  }
};

inline static Interceptor gInterceptor;

inline Interceptor *Interceptor::Shared() {
  return &gInterceptor;
}