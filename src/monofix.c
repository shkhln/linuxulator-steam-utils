#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

/* https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=253337 */

static int (*libc_mprotect)(void *addr, size_t len, int prot) = NULL;

int mprotect(void *addr, size_t len, int prot) {

  if (!libc_mprotect) {
    libc_mprotect = dlsym(RTLD_NEXT, "mprotect");
  }

  if (prot == PROT_NONE /*&& (uintptr_t)addr > (uintptr_t)0x7f0000000000*/) {

    void* buffer[2];
    int nframes = backtrace(buffer, 2);
    assert(nframes == 2);

    char* caller_str = *backtrace_symbols(buffer + 1, 1);
    assert(caller_str != NULL);

    char* p = strrchr(caller_str, '/');
    if (p && strncmp("libmonobdwgc-2.0.so", p + 1, sizeof("libmonobdwgc-2.0.so") - 1) == 0) {
      return 0;
    }
  }

  return libc_mprotect(addr, len, prot);
}

/* https://github.com/mono/eglib/blob/37edc457aead8c3d4a39a929d0ecf6f25790b544/src/gfile-posix.c#L173 */

static char* (*libc_getcwd)(char*, size_t) = NULL;

char* getcwd(char* buf, size_t size) {

  if (!libc_getcwd) {
    libc_getcwd = dlsym(RTLD_NEXT, "getcwd");
  }

  char* s = libc_getcwd(buf, size);
  if (s == NULL && errno == ENOMEM) {
    errno = ERANGE;
  }

  return s;
}
