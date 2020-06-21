#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* substitute wine with wine64, skip steam.so.exe helper */

#ifdef __i386__

extern char* __progname;

__attribute__((constructor))
static void redirect_wine_to_wine64(int argc, char** argv, char** env) {

  if (getenv("PROTONFIX_REDIRECT") != NULL) {
    exit(1);
  }

  if (strcmp(__progname, "wine") == 0) {

    char wine64_path[strlen(argv[0]) + 3];
    snprintf(wine64_path, sizeof(wine64_path), "%s64", argv[0]);

    if (strcmp(argv[1], "steam") == 0) {
      argv[0] = "/compat/linux/bin/env";
      argv[1] = wine64_path;
    } else {
      argv[0] = wine64_path;
    }

    setenv("PROTONFIX_REDIRECT", "1", 1);

    execv(argv[0], argv);

    perror("execv");
    exit(1);
  }
}

unsigned short wine_ldt_alloc_fs() {
  assert(0);
}

#endif

/* ??? */

#ifdef __x86_64__

#define PTRACE_POKEDATA 5

static long (*libc_ptrace)(int, pid_t, void*, void*) = NULL;

long ptrace(int request, pid_t pid, void* addr, void* data) {

  if (!libc_ptrace) {
    libc_ptrace = dlsym(RTLD_NEXT, "ptrace");
  }

  if (request == PTRACE_POKEDATA) {
    fprintf(stderr, "PTRACE_POKEDATA: addr = %p, data = %p\n", addr, data);
    return -1;
  } else {
    return libc_ptrace(request, pid, addr, data);
  }
}

#endif
