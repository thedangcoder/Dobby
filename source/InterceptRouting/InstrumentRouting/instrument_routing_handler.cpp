#include "dobby/dobby_internal.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"
#include "InterceptRouting/InstrumentRouting.h"
#include "MultiThreadSupport/ThreadSupport.h"
#include "logging/logging.h"

void instrument_forward_handler(Interceptor::Entry *entry, DobbyRegisterContext *ctx);

extern "C" void instrument_epilogue_dispatch(Interceptor::Entry *entry, DobbyRegisterContext *ctx);

extern "C" void instrument_routing_dispatch(Interceptor::Entry *entry, DobbyRegisterContext *ctx) {
  // __FUNC_CALL_TRACE__();

  // Call pre-handler if exists
  auto pre_callback = (dobby_instrument_callback_t)entry->pre_handler;
  if (pre_callback) {
    pre_callback((void *)entry->addr, ctx);
  }

  // If post-handler exists, setup return address hijacking
  if (entry->post_handler && entry->epilogue_dispatch_bridge) {
    StackFrame *stackframe = new StackFrame();
    stackframe->orig_ret = get_func_ret_address(ctx);
    ThreadSupport::PushStackFrame(stackframe);

    // Replace return address with our epilogue trampoline
    set_func_ret_address(ctx, (void *)entry->epilogue_dispatch_bridge);
  }

  // set TMP_REG_0 to next hop (relocated original code)
  set_routing_bridge_next_hop(ctx, (void *)entry->relocated.addr());
}

extern "C" void instrument_epilogue_dispatch(Interceptor::Entry *entry, DobbyRegisterContext *ctx) {
  // Pop the saved stack frame
  StackFrame *stackframe = ThreadSupport::PopStackFrame();

  // Call post-handler with full register context (includes return value)
  auto post_callback = (dobby_instrument_callback_t)entry->post_handler;
  if (post_callback) {
    post_callback((void *)entry->addr, ctx);
  }

  // Restore original return address as next hop
  set_routing_bridge_next_hop(ctx, stackframe->orig_ret);

  delete stackframe;
}
