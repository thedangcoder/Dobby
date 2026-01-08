# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dobby is a lightweight, multi-platform, multi-architecture inline hook framework. It supports:
- Platforms: Windows, macOS, iOS, Android, Linux
- Architectures: x86, x86-64, ARM, ARM64

## Build Commands

### Standard Host Build
```bash
mkdir cmake-build && cd cmake-build
cmake ..
cmake --build . -j4
```

### Cross-Platform Build (using platform_builder.py)
```bash
# iOS
python3 scripts/platform_builder.py --platform=iphoneos --arch=all

# macOS
python3 scripts/platform_builder.py --platform=macos --arch=all

# Android (requires NDK)
python3 scripts/platform_builder.py --platform=android --arch=all --android_ndk_dir=$NDK_PATH

# Linux cross-compile
python3 scripts/platform_builder.py --platform=linux --arch=all
```

### Key CMake Options
```
-DDOBBY_DEBUG=ON/OFF        # Enable debug logging (default: ON in Debug builds)
-DNearBranch=ON/OFF         # Near branch trampoline (default: ON)
-DPlugin.SymbolResolver=ON  # Symbol resolver plugin (default: ON)
-DDOBBY_BUILD_TEST=ON       # Build tests (requires capstone + unicorn)
-DDOBBY_BUILD_EXAMPLE=ON    # Build examples
```

### Building Tests
Tests require `capstone` and `unicorn` libraries installed via pkg-config:
```bash
cmake .. -DDOBBY_BUILD_TEST=ON
cmake --build .
./test_native           # Native hook test
./test_insn_relo_arm64  # ARM64 instruction relocation test
./test_insn_relo_x64    # x64 instruction relocation test
```

## Architecture Overview

### Core Components

**Public API** (`include/dobby.h`):
- `DobbyHook(address, fake_func, &orig_func)` - Inline function hooking
- `DobbyInstrument(address, callback)` - Dynamic binary instrumentation
- `DobbyDestroy(address)` - Remove hook and restore original code
- `DobbySymbolResolver(image, symbol)` - Resolve symbol addresses
- `DobbyCodePatch(address, buffer, size)` - Low-level memory patching

**Interceptor** (`source/Interceptor.h`):
Global singleton managing all hook entries. Each `Entry` stores:
- Original address and patched code range
- Relocated code block (original instructions moved to executable memory)
- Routing information (trampoline pointers)

**InterceptRouting** (`source/InterceptRouting/`):
- `InlineHookRouting` - Standard function replacement hooking
- `InstrumentRouting` - Instrumentation with callback before execution
- Both generate trampolines and handle instruction relocation

### Hook Flow
1. Create `Interceptor::Entry` for target address
2. Build routing (InlineHook or Instrument)
3. Generate trampoline code (near or normal based on distance)
4. Relocate original instructions to new executable memory
5. Patch original location with jump to trampoline

### Platform Backends (`source/Backend/UserMode/`)
- `ExecMemory/` - Platform-specific executable memory allocation and patching
- `PlatformUtil/` - Process runtime information (memory maps, module info)
- `UnifiedInterface/` - Abstraction layer (`platform-posix.cc`, `platform-windows.cc`)

### Instruction Relocation (`source/InstructionRelocation/`)
Per-architecture instruction analysis and relocation:
- Handles PC-relative instructions that break when moved
- Generates equivalent instruction sequences at new locations
- `arm/`, `arm64/`, `x86/`, `x64/` subdirectories

### Trampoline Bridge (`source/TrampolineBridge/`)
- `Trampoline/` - Jump stubs from hook site to handler
- `ClosureTrampolineBridge/` - Context-saving bridges for instrumentation callbacks

### Memory Allocation (`source/MemoryAllocator/`)
- `NearMemoryAllocator` - Allocates memory within ±128MB for near branches
- `MemoryAllocator` - General executable memory allocation

### Plugins (`builtin-plugin/`)
- `SymbolResolver/` - Symbol lookup (ELF, Mach-O, PE formats)
- `ImportTableReplace/` - IAT/GOT hooking
- `ObjcRuntimeReplace/` - Objective-C method swizzling

## Code Conventions

- C++17 standard
- Architecture detection via macros: `__arm__`, `__arm64__`/`__aarch64__`, `__i386__`/`_M_IX86`, `__x86_64__`/`_M_X64`
- Platform detection: `SYSTEM.Darwin`, `SYSTEM.Linux`, `SYSTEM.Android`, `SYSTEM.Windows`
- Use `PUBLIC` macro for exported symbols
- Debug logging via `DEBUG_LOG()`, `ERROR_LOG()` macros
- Function tracing with `__FUNC_CALL_TRACE__()` macro
