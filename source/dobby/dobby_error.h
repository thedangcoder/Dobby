#pragma once

// Include the public header which defines DobbyError enum
#include "dobby.h"

// Legacy compatibility - kMemoryOperationError was used in some files
#define kMemoryOperationError kDobbyErrorMemoryOperation

// Helper macro to check if result is success
#define DOBBY_SUCCESS(result) ((result) == kDobbySuccess)
#define DOBBY_FAILED(result) ((result) != kDobbySuccess)

// Thread-local last error storage (optional, for detailed error info)
#ifdef __cplusplus
extern "C" {
#endif

// Get the last error code (thread-local)
DobbyError DobbyGetLastError(void);

// Get a human-readable error message for an error code
const char *DobbyErrorString(DobbyError error);

#ifdef __cplusplus
}
#endif
