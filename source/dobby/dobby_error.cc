#include "dobby/dobby_error.h"

#if defined(_WIN32)
#include <windows.h>
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

// Thread-local storage for the last error
static THREAD_LOCAL DobbyError g_last_error = kDobbySuccess;

extern "C" {

DobbyError DobbyGetLastError(void) {
  return g_last_error;
}

void DobbySetLastError(DobbyError error) {
  g_last_error = error;
}

const char *DobbyErrorString(DobbyError error) {
  switch (error) {
  case kDobbySuccess:
    return "Success";

  // General errors
  case kDobbyErrorInvalidArgument:
    return "Invalid argument (NULL pointer or invalid parameter)";
  case kDobbyErrorNotFound:
    return "Hook or entry not found";
  case kDobbyErrorAlreadyExists:
    return "Address already hooked or instrumented";
  case kDobbyErrorNotSupported:
    return "Operation not supported on this platform or architecture";
  case kDobbyErrorUnknown:
    return "Unknown error";

  // Memory errors
  case kDobbyErrorMemoryAllocation:
    return "Memory allocation failed";
  case kDobbyErrorMemoryProtection:
    return "Failed to change memory protection";
  case kDobbyErrorMemoryOperation:
    return "Memory operation failed";
  case kDobbyErrorNearMemoryExhausted:
    return "No near memory available for trampoline";

  // Code relocation errors
  case kDobbyErrorRelocationFailed:
    return "Instruction relocation failed";
  case kDobbyErrorUnsupportedInstruction:
    return "Cannot relocate unsupported instruction";
  case kDobbyErrorCodeTooShort:
    return "Not enough bytes available to patch";

  // Routing errors
  case kDobbyErrorTrampolineGeneration:
    return "Failed to generate trampoline";
  case kDobbyErrorRoutingBuild:
    return "Failed to build routing";

  default:
    return "Unknown error code";
  }
}

} // extern "C"
