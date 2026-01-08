#pragma once

#include "dobby.h"
#include "dobby/types.h"
#include "dobby/dobby_error.h"
#include "dobby/platform_features.h"
#include "dobby/platform_detect_macro.h"
#include "dobby/utility_macro.h"

#include "logging/logging.h"
#include "logging/check_logging.h"

#include "common/os_arch_features.h"
#include "common/hex_log.h"
#include "common/pac_kit.h"

// Internal helper to set last error and return it
#define DOBBY_RETURN_ERROR(err) \
  do { \
    DobbySetLastError(err); \
    return err; \
  } while (0)

// Declaration for internal use
extern "C" void DobbySetLastError(DobbyError error);
