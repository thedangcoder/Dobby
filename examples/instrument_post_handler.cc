#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dobby.h"

// Pre-handler: called before function executes
void pre_handler(void *address, DobbyRegisterContext *ctx) {
#if defined(__x86_64__) || defined(_M_X64)
#ifdef _WIN32
  printf("[pre]  strlen called: str=%s\n", (char *)ctx->general.regs.rcx);
#else
  printf("[pre]  strlen called: str=%s\n", (char *)ctx->general.regs.rdi);
#endif
#elif defined(__aarch64__) || defined(__arm64__)
  printf("[pre]  strlen called: str=%s\n", (char *)ctx->general.regs.x0);
#elif defined(__arm__)
  printf("[pre]  strlen called: str=%s\n", (char *)ctx->general.regs.r0);
#elif defined(__i386__) || defined(_M_IX86)
  printf("[pre]  strlen called at %p\n", address);
#endif
}

// Post-handler: called after function executes
void post_handler(void *address, DobbyRegisterContext *ctx) {
#if defined(__x86_64__) || defined(_M_X64)
  printf("[post] strlen returned: %lu\n", (unsigned long)ctx->general.regs.rax);
#elif defined(__aarch64__) || defined(__arm64__)
  printf("[post] strlen returned: %lu\n", (unsigned long)ctx->general.regs.x0);
#elif defined(__arm__)
  printf("[post] strlen returned: %lu\n", (unsigned long)ctx->general.regs.r0);
#elif defined(__i386__) || defined(_M_IX86)
  printf("[post] strlen returned: %lu\n", (unsigned long)ctx->general.regs.eax);
#endif
}

int main() {
  printf("=== DobbyInstrument Post-Handler Test ===\n\n");

  // Resolve strlen from libc
  void *strlen_addr = DobbySymbolResolver(NULL, "strlen");
  printf("strlen address: %p\n", strlen_addr);
  fflush(stdout);

  if (!strlen_addr) {
    printf("Failed to resolve strlen!\n");
    return 1;
  }

  printf("Before instrumentation:\n");
  fflush(stdout);
  size_t result1 = strlen("Hello World");
  printf("strlen(\"Hello World\") = %zu\n\n", result1);
  fflush(stdout);

  printf("Installing instrumentation with pre and post handlers...\n");
  fflush(stdout);

  int ret = DobbyInstrumentEx(strlen_addr, pre_handler, post_handler);
  if (ret != 0) {
    printf("DobbyInstrument failed with error: %s\n", DobbyErrorString((DobbyError)ret));
    return 1;
  }
  printf("Instrumentation installed successfully!\n\n");
  fflush(stdout);

  printf("After instrumentation:\n");
  fflush(stdout);
  size_t result2 = strlen("Test String");
  printf("strlen(\"Test String\") = %zu\n\n", result2);
  fflush(stdout);

  printf("Calling again:\n");
  fflush(stdout);
  size_t result3 = strlen("ABC");
  printf("strlen(\"ABC\") = %zu\n\n", result3);

  printf("Removing instrumentation...\n");
  DobbyDestroy(strlen_addr);

  printf("After removing instrumentation:\n");
  size_t result4 = strlen("Done");
  printf("strlen(\"Done\") = %zu\n\n", result4);

  printf("=== Test Complete ===\n");
  return 0;
}
