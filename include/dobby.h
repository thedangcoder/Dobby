/**
 * @file dobby.h
 * @brief Dobby - A lightweight, multi-platform, multi-architecture inline hook framework
 *
 * Dobby provides function hooking and dynamic binary instrumentation capabilities
 * across multiple platforms (Windows, macOS, iOS, Android, Linux) and architectures
 * (x86, x86-64, ARM, ARM64).
 *
 * @section features Key Features
 * - Inline function hooking with automatic instruction relocation
 * - Dynamic binary instrumentation with register context access
 * - Symbol resolution for dynamic libraries
 * - Import table (IAT/GOT) hooking
 * - Thread-safe operations
 *
 * @section usage Basic Usage
 * @code
 * // Hook a function
 * void *orig_func;
 * int ret = DobbyHook(target_addr, my_hook_func, &orig_func);
 * if (DOBBY_FAILED(ret)) {
 *     printf("Hook failed: %s\n", DobbyErrorString(ret));
 * }
 *
 * // Instrument a function (inspect without replacing)
 * DobbyInstrument(target_addr, my_callback);
 *
 * // Remove a hook
 * DobbyDestroy(target_addr);
 * @endcode
 *
 * @copyright Copyright (c) Dobby Authors
 * @license Apache-2.0
 */

#ifndef dobby_h
#define dobby_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/** @brief Address type matching pointer size of target platform */
typedef uintptr_t addr_t;

/** @brief 32-bit address type */
typedef uint32_t addr32_t;

/** @brief 64-bit address type */
typedef uint64_t addr64_t;

/** @brief Assembly function pointer type */
typedef void *asm_func_t;

/* ============================================================================
 * Error Codes
 * ============================================================================ */

/**
 * @brief Error codes returned by Dobby API functions
 *
 * All Dobby functions return these error codes. Success is always 0 (kDobbySuccess).
 * Errors are negative values grouped by category:
 * - General errors: -1 to -99
 * - Memory errors: -100 to -199
 * - Code relocation errors: -200 to -299
 * - Routing errors: -300 to -399
 */
typedef enum DobbyError {
  /** @brief Operation completed successfully */
  kDobbySuccess = 0,

  /* General errors (-1 to -99) */
  /** @brief NULL pointer or invalid parameter passed */
  kDobbyErrorInvalidArgument = -1,
  /** @brief Hook entry not found at specified address */
  kDobbyErrorNotFound = -2,
  /** @brief Address is already hooked or instrumented */
  kDobbyErrorAlreadyExists = -3,
  /** @brief Operation not supported on current platform/architecture */
  kDobbyErrorNotSupported = -4,
  /** @brief Unknown or unspecified error */
  kDobbyErrorUnknown = -5,

  /* Memory errors (-100 to -199) */
  /** @brief Failed to allocate memory */
  kDobbyErrorMemoryAllocation = -100,
  /** @brief Failed to change memory protection (mprotect/VirtualProtect) */
  kDobbyErrorMemoryProtection = -101,
  /** @brief General memory operation error */
  kDobbyErrorMemoryOperation = -102,
  /** @brief No memory available within near branch range (±128MB) */
  kDobbyErrorNearMemoryExhausted = -103,

  /* Code relocation errors (-200 to -299) */
  /** @brief Failed to relocate instructions */
  kDobbyErrorRelocationFailed = -200,
  /** @brief Encountered instruction that cannot be relocated */
  kDobbyErrorUnsupportedInstruction = -201,
  /** @brief Not enough bytes at target address to patch */
  kDobbyErrorCodeTooShort = -202,

  /* Routing errors (-300 to -399) */
  /** @brief Failed to generate trampoline code */
  kDobbyErrorTrampolineGeneration = -300,
  /** @brief Failed to build hook routing */
  kDobbyErrorRoutingBuild = -301,
} DobbyError;

/**
 * @brief Check if a Dobby operation succeeded
 * @param result Return value from a Dobby function
 * @return true if operation succeeded, false otherwise
 */
#define DOBBY_SUCCESS(result) ((result) == kDobbySuccess)

/**
 * @brief Check if a Dobby operation failed
 * @param result Return value from a Dobby function
 * @return true if operation failed, false otherwise
 */
#define DOBBY_FAILED(result) ((result) != kDobbySuccess)

/**
 * @brief Get the last error code for the current thread
 *
 * Each thread maintains its own last error value. This function retrieves
 * the error code set by the most recent Dobby API call in the current thread.
 *
 * @return The last error code, or kDobbySuccess if no error occurred
 */
DobbyError DobbyGetLastError(void);

/**
 * @brief Get a human-readable description for an error code
 *
 * @param error The error code to describe
 * @return A constant string describing the error. Never returns NULL.
 *
 * @note The returned string is statically allocated and should not be freed.
 */
const char *DobbyErrorString(DobbyError error);

/* ============================================================================
 * Register Context (Architecture-Specific)
 * ============================================================================ */

#if defined(__arm__)
/**
 * @brief ARM32 register context for instrumentation callbacks
 *
 * Contains general-purpose registers r0-r12, sp, and lr.
 * Used in DobbyInstrument callbacks to inspect/modify register state.
 */
typedef struct {
  uint32_t dummy_0;
  uint32_t dummy_1;
  uint32_t dummy_2;
  uint32_t sp;          /**< Stack pointer */

  union {
    uint32_t r[13];     /**< General registers as array */
    struct {
      uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
    } regs;             /**< General registers by name */
  } general;

  uint32_t lr;          /**< Link register */
} DobbyRegisterContext;

#elif defined(__arm64__) || defined(__aarch64__)
/** @brief Index of temporary register used internally */
#define ARM64_TMP_REG_NDX_0 17

/**
 * @brief ARM64 floating-point register
 */
typedef union _FPReg {
  __int128_t q;         /**< 128-bit quadword view */
  struct {
    double d1;
    double d2;
  } d;                  /**< Double-precision view */
  struct {
    float f1;
    float f2;
    float f3;
    float f4;
  } f;                  /**< Single-precision view */
} FPReg;

/**
 * @brief ARM64 register context for instrumentation callbacks
 *
 * Contains general-purpose registers x0-x28, sp, fp, lr, and floating-point
 * registers q0-q31.
 *
 * @note By default, only q0-q7 are saved/restored. Enable full floating-point
 *       register pack with cmake flag -DFullFloatingPointRegisterPack=ON to
 *       access q8-q31.
 */
typedef struct {
  uint64_t dmmpy_0;     /**< Internal padding */
  uint64_t sp;          /**< Stack pointer */

  uint64_t dmmpy_1;     /**< Internal padding */
  union {
    uint64_t x[29];     /**< General registers as array */
    struct {
      uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
               x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
    } regs;             /**< General registers by name */
  } general;

  uint64_t fp;          /**< Frame pointer (x29) */
  uint64_t lr;          /**< Link register (x30) */

  union {
    FPReg q[32];        /**< FP registers as array */
    struct {
      FPReg q0, q1, q2, q3, q4, q5, q6, q7;
      /* q8-q31 require FullFloatingPointRegisterPack=ON */
      FPReg q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20, q21,
            q22, q23, q24, q25, q26, q27, q28, q29, q30, q31;
    } regs;             /**< FP registers by name */
  } floating;
} DobbyRegisterContext;

#elif defined(_M_IX86) || defined(__i386__)
/**
 * @brief x86 (32-bit) register context for instrumentation callbacks
 */
typedef struct _RegisterContext {
  uint32_t dummy_0;
  uint32_t esp;         /**< Stack pointer */

  uint32_t dummy_1;
  uint32_t flags;       /**< EFLAGS register */

  union {
    struct {
      uint32_t eax, ebx, ecx, edx, ebp, esp, edi, esi;
    } regs;             /**< General registers by name */
  } general;
} DobbyRegisterContext;

#elif defined(_M_X64) || defined(__x86_64__)
/**
 * @brief x86-64 register context for instrumentation callbacks
 */
typedef struct {
  union {
    struct {
      uint64_t rax, rbx, rcx, rdx, rbp, rsp, rdi, rsi, r8, r9, r10, r11, r12,
               r13, r14, r15;
    } regs;             /**< General registers by name */
  } general;

  uint64_t dummy_0;
  uint64_t flags;       /**< RFLAGS register */
  uint64_t ret;         /**< Return address */
} DobbyRegisterContext;
#endif

/* ============================================================================
 * Convenience Macros
 * ============================================================================ */

/**
 * @brief Helper macro to define and install a hook
 *
 * This macro creates the necessary boilerplate for defining a hook function
 * and its original function pointer.
 *
 * @param name Base name for the hook (creates fake_##name and orig_##name)
 * @param fn_ret_t Return type of the hooked function
 * @param ... Argument types of the hooked function
 *
 * @code
 * install_hook_name(malloc, void*, size_t size) {
 *     printf("malloc called with size %zu\n", size);
 *     return orig_malloc(size);
 * }
 *
 * // Later, install the hook:
 * install_hook_malloc(malloc_addr);
 * @endcode
 */
#define install_hook_name(name, fn_ret_t, ...)                                 \
  static fn_ret_t fake_##name(__VA_ARGS__);                                    \
  static fn_ret_t (*orig_##name)(__VA_ARGS__);                                 \
  /* __attribute__((constructor)) */                                           \
  static void install_hook_##name(void *sym_addr) {                            \
    DobbyHook(sym_addr, (void *)fake_##name, (void **)&orig_##name);           \
    return;                                                                    \
  }                                                                            \
  fn_ret_t fake_##name(__VA_ARGS__)

/* ============================================================================
 * Core API Functions
 * ============================================================================ */

/**
 * @brief Patch memory with new bytes
 *
 * Low-level function to write arbitrary bytes to a memory address.
 * Handles memory protection changes automatically.
 *
 * @param address Target address to patch
 * @param buffer  Pointer to bytes to write
 * @param buffer_size Number of bytes to write
 *
 * @return kDobbySuccess on success, or error code:
 *         - kDobbyErrorInvalidArgument: NULL address or buffer, or zero size
 *         - kDobbyErrorMemoryProtection: Failed to change memory permissions
 *
 * @warning This is a low-level function. For function hooking, use DobbyHook().
 */
int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size);

/**
 * @brief Install an inline hook on a function
 *
 * Replaces the target function with a hook function. The original function
 * can still be called through the returned trampoline pointer.
 *
 * @param address Target function address to hook
 * @param fake_func Replacement function (must have same signature)
 * @param out_origin_func [out] Receives pointer to call original function.
 *                        Can be NULL if original doesn't need to be called.
 *
 * @return kDobbySuccess on success, or error code:
 *         - kDobbyErrorInvalidArgument: NULL address
 *         - kDobbyErrorAlreadyExists: Address already hooked
 *         - kDobbyErrorRoutingBuild: Failed to build hook (relocation failed)
 *
 * @code
 * static int (*orig_open)(const char*, int);
 *
 * int my_open(const char *path, int flags) {
 *     printf("Opening: %s\n", path);
 *     return orig_open(path, flags);
 * }
 *
 * // Install hook
 * DobbyHook(open, my_open, (void**)&orig_open);
 * @endcode
 *
 * @note Thread-safe. Can be called from multiple threads.
 * @see DobbyDestroy() to remove the hook
 */
int DobbyHook(void *address, void *fake_func, void **out_origin_func);

/**
 * @brief Callback type for instrumentation
 *
 * @param address The instrumented address being executed
 * @param ctx Register context at time of execution (can be modified)
 */
typedef void (*dobby_instrument_callback_t)(void *address, DobbyRegisterContext *ctx);

/**
 * @brief Instrument a function for dynamic analysis
 *
 * Installs a callback that is invoked before the target function executes.
 * Unlike DobbyHook, the original function always executes after the callback.
 *
 * @param address Target function address to instrument
 * @param pre_handler Callback invoked before function execution
 *
 * @return kDobbySuccess on success, or error code:
 *         - kDobbyErrorInvalidArgument: NULL address
 *         - kDobbyErrorAlreadyExists: Address already instrumented
 *         - kDobbyErrorRoutingBuild: Failed to build instrumentation
 *
 * @code
 * void trace_function(void *addr, DobbyRegisterContext *ctx) {
 *     printf("Function at %p called, arg1=%p\n", addr, (void*)ctx->general.regs.rdi);
 * }
 *
 * DobbyInstrument(target_func, trace_function);
 * @endcode
 *
 * @note On ARM64, floating-point registers q8-q31 are only available if
 *       compiled with -DFullFloatingPointRegisterPack=ON
 * @see DobbyDestroy() to remove the instrumentation
 */
int DobbyInstrument(void *address, dobby_instrument_callback_t pre_handler);

/**
 * @brief Instrument a function with pre and post execution callbacks
 *
 * Installs callbacks that are invoked before and after the target function executes.
 * Unlike DobbyHook, the original function always executes between the callbacks.
 * The post_handler receives the full register context including return value.
 *
 * @param address Target function address to instrument
 * @param pre_handler Callback invoked before function execution (can be NULL)
 * @param post_handler Callback invoked after function execution (can be NULL)
 *
 * @return kDobbySuccess on success, or error code:
 *         - kDobbyErrorInvalidArgument: NULL address or both handlers NULL
 *         - kDobbyErrorAlreadyExists: Address already instrumented
 *         - kDobbyErrorRoutingBuild: Failed to build instrumentation
 *
 * @code
 * void pre_handler(void *addr, DobbyRegisterContext *ctx) {
 *     printf("Function called with arg1=%p\n", (void*)ctx->general.regs.rdi);
 * }
 *
 * void post_handler(void *addr, DobbyRegisterContext *ctx) {
 *     printf("Function returned %p\n", (void*)ctx->general.regs.rax);
 * }
 *
 * DobbyInstrumentEx(target_func, pre_handler, post_handler);
 * @endcode
 *
 * @note Requires thread-local storage for tracking return addresses
 * @note Post-handler may not be called if function uses tail call optimization
 * @see DobbyDestroy() to remove the instrumentation
 */
int DobbyInstrumentEx(void *address,
                      dobby_instrument_callback_t pre_handler,
                      dobby_instrument_callback_t post_handler);

/**
 * @brief Remove a hook or instrumentation and restore original code
 *
 * @param address Address that was previously hooked or instrumented
 *
 * @return kDobbySuccess on success, or error code:
 *         - kDobbyErrorInvalidArgument: NULL address
 *         - kDobbyErrorNotFound: No hook/instrumentation at address
 *
 * @note After calling this, any saved original function pointers become invalid.
 */
int DobbyDestroy(void *address);

/**
 * @brief Get Dobby library version string
 *
 * @return Version string in format "Dobby-YYYYMMDD-gitcommit"
 *
 * @code
 * printf("Using %s\n", DobbyGetVersion());
 * // Output: "Using Dobby-20240115-abc1234"
 * @endcode
 */
const char *DobbyGetVersion(void);

/* ============================================================================
 * Symbol Resolution
 * ============================================================================ */

/**
 * @brief Resolve a symbol address from a loaded image
 *
 * Searches for a symbol in the specified image (shared library/DLL).
 *
 * @param image_name Name or path of the image. Can be:
 *                   - Full path: "/usr/lib/libc.so.6"
 *                   - Library name: "libc.so" (searches loaded libraries)
 *                   - NULL: Search all loaded images
 * @param symbol_name Name of the symbol to find
 *
 * @return Address of the symbol, or NULL if not found
 *
 * @code
 * void *malloc_addr = DobbySymbolResolver("libc.so", "malloc");
 * void *objc_msg = DobbySymbolResolver("libobjc.dylib", "objc_msgSend");
 * @endcode
 *
 * @note On macOS/iOS, supports Mach-O format
 * @note On Linux/Android, supports ELF format
 * @note On Windows, requires Plugin.SymbolResolver (PE support limited)
 */
void *DobbySymbolResolver(const char *image_name, const char *symbol_name);

/* ============================================================================
 * Import Table Hooking
 * ============================================================================ */

/**
 * @brief Replace a function in the import table (IAT/GOT)
 *
 * Hooks a function by modifying the import table rather than inline patching.
 * This is useful when inline hooking is not possible or desired.
 *
 * @param image_name Image whose import table to modify
 * @param symbol_name Name of the imported symbol to replace
 * @param fake_func Replacement function
 * @param orig_func [out] Receives original function pointer. Can be NULL.
 *
 * @return kDobbySuccess on success, or error code
 *
 * @note Requires Plugin.ImportTableReplace=ON at compile time
 * @note Only affects imports in the specified image, not all callers
 */
int DobbyImportTableReplace(char *image_name, char *symbol_name, void *fake_func, void **orig_func);

/* ============================================================================
 * Configuration Functions
 * ============================================================================ */

/**
 * @brief Enable or disable near branch trampolines
 *
 * Near branches use shorter jump instructions that can only reach ±128MB.
 * They are faster and use less memory, but may fail if no memory is available
 * in the required range.
 *
 * @param enable true to prefer near branches (default), false for absolute jumps
 *
 * @note On x86/x64, absolute jumps are always used regardless of this setting
 * @note On ARM/ARM64, near branches use B instruction instead of LDR+BR
 */
void dobby_set_near_trampoline(bool enable);

/**
 * @brief Callback type for custom near memory allocation
 *
 * @param size   Required allocation size
 * @param pos    Target address (trampoline should be within range of this)
 * @param range  Maximum distance from pos (typically 128MB for near branches)
 *
 * @return Address of allocated memory, or 0 on failure
 */
typedef addr_t (*dobby_alloc_near_code_callback_t)(uint32_t size, addr_t pos, size_t range);

/**
 * @brief Register a custom allocator for near code blocks
 *
 * Allows providing a custom memory allocator for trampoline code. Useful when
 * the default allocator cannot find suitable memory (e.g., in restricted
 * environments).
 *
 * @param handler Allocation callback, or NULL to use default allocator
 */
void dobby_register_alloc_near_code_callback(dobby_alloc_near_code_callback_t handler);

/**
 * @brief Configure Dobby options
 *
 * Convenience function to set multiple options at once.
 *
 * @param enable_near_trampoline Enable near branch trampolines
 * @param alloc_near_code_callback Custom memory allocator (can be NULL)
 *
 * @see dobby_set_near_trampoline()
 * @see dobby_register_alloc_near_code_callback()
 */
void dobby_set_options(bool enable_near_trampoline, dobby_alloc_near_code_callback_t alloc_near_code_callback);

#ifdef __cplusplus
}
#endif

#endif /* dobby_h */
