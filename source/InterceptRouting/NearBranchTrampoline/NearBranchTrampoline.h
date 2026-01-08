#pragma once

#include "dobby/dobby_internal.h"

#include "InterceptRouting/RoutingPlugin.h"
#include <atomic>

inline std::atomic<bool> g_enable_near_trampoline{false};

PUBLIC extern "C" inline void dobby_set_near_trampoline(bool enable) {
  g_enable_near_trampoline.store(enable, std::memory_order_relaxed);
}

inline bool is_near_trampoline_enabled() {
  return g_enable_near_trampoline.load(std::memory_order_relaxed);
}

class NearBranchTrampolinePlugin : public RoutingPluginInterface {};