#include <stdlib.h> /* getenv */
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include <set>
#include <unordered_map>

#include <sys/param.h>

#include "./dobby_monitor.h"

std::unordered_map<FILE *, const char *> *TracedFopenFileList;

// Safe string copy with bounds checking
static char *safe_strdup(const char *src, size_t max_len) {
  if (!src) return nullptr;
  size_t len = strlen(src);
  if (len >= max_len) len = max_len - 1;
  char *dst = (char *)malloc(len + 1);
  if (dst) {
    memcpy(dst, src, len);
    dst[len] = '\0';
  }
  return dst;
}

FILE *(*orig_fopen)(const char *filename, const char *mode);
FILE *fake_fopen(const char *filename, const char *mode) {
  FILE *result = NULL;
  result = orig_fopen(filename, mode);
  if (result != NULL && filename) {
    char *traced_filename = safe_strdup(filename, MAXPATHLEN);
    if (traced_filename) {
      std::cout << "[-] trace file: " << filename << std::endl;
      TracedFopenFileList->insert(std::make_pair(result, traced_filename));
    }
  }
  return result;
}

static const char *GetFileDescriptorTraced(FILE *stream, bool removed) {
  if (!TracedFopenFileList)
    return NULL;
  std::unordered_map<FILE *, const char *>::iterator it;
  it = TracedFopenFileList->find(stream);
  if (it != TracedFopenFileList->end()) {
    const char *filename = it->second;
    if (removed)
      TracedFopenFileList->erase(it);
    return filename;
  }
  return NULL;
}

int (*orig_fclose)(FILE *stream);
int fake_fclose(FILE *stream) {
  const char *traced_filename = GetFileDescriptorTraced(stream, true);
  if (traced_filename) {
    LOG("[-] fclose: %s\n", traced_filename);
    free((void *)traced_filename);
  }
  return orig_fclose(stream);
}

size_t (*orig_fread)(void *ptr, size_t size, size_t count, FILE *stream);
size_t fake_fread(void *ptr, size_t size, size_t count, FILE *stream) {
  const char *file_name = GetFileDescriptorTraced(stream, false);
  if (file_name) {
    LOG("[-] fread: %s, buffer: %p\n", file_name, ptr);
  }
  return orig_fread(ptr, size, count, stream);
}

size_t (*orig_fwrite)(const void *ptr, size_t size, size_t count, FILE *stream);
size_t fake_fwrite(void *ptr, size_t size, size_t count, FILE *stream) {
  const char *file_name = GetFileDescriptorTraced(stream, false);
  if (file_name) {
    LOG("[-] fwrite %s\n    from %p\n", file_name, ptr);
  }
  return orig_fwrite(ptr, size, count, stream);
}

__attribute__((constructor)) void __main() {

  TracedFopenFileList = new std::unordered_map<FILE *, const char *>();

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if (TARGET_OS_IPHONE || TARGET_OS_MAC)
  std::ifstream file;
  file.open("/System/Library/CoreServices/SystemVersion.plist");
  std::cout << file.rdbuf();
#endif
#endif

  //   DobbyHook((void *)fopen, (void *)fake_fopen, (void **)&orig_fopen);
  //   DobbyHook((void *)fwrite, (void *)fake_fwrite, (void **)&orig_fwrite);
  //   DobbyHook((void *)fread, (void *)fake_fread, (void **)&orig_fread);

  char *home = getenv("HOME");
  char *subdir = (char *)"/Library/Caches/";

  std::string filePath = std::string(home) + std::string(subdir) + "temp.log";

  char buffer[64];
  memset(buffer, 'B', 64);

  FILE *fd = fopen(filePath.c_str(), "w+");
  if (!fd)
    std::cout << "[!] open " << filePath << "failed!\n";

  fwrite(buffer, 64, 1, fd);
  fflush(fd);
  fseek(fd, 0, SEEK_SET);
  memset(buffer, 0, 64);

  fread(buffer, 64, 1, fd);

  return;
}
