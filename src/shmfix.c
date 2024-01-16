#define _GNU_SOURCE

#if 0

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

__attribute__((constructor))
static void init() {
  fprintf(stderr, "[[%d: loaded %s]]\n", getpid(), __FILE__);
}

static int (*libc_shm_open)(const char*, int, mode_t) = NULL;

int shm_open(const char* path, int flags, mode_t mode) {

  if (!libc_shm_open) {
    libc_shm_open = dlsym(RTLD_NEXT, "shm_open");
    assert(libc_shm_open != shm_open);
    assert(libc_shm_open != NULL);
  }

  fprintf(stderr, "%d: %s(%s, 0x%x, 0%o)\n", getpid(), __func__, path, flags, mode);

  int fd = libc_shm_open(path, flags, mode);
  if (fd == -1) {
    perror("shm_open");
  }

  fprintf(stderr, "%d: %s -> %d\n", getpid(), __func__, fd);

  return fd;
}

#endif

/* glibc < 2.34 doesn't like seeing nullfs at /dev/shm and returns NULL from __shm_directory */

#include <stdlib.h>

#define SHM_DIR "/dev/shm/"

const char* __shm_directory(size_t* len) {
  *len = sizeof(SHM_DIR) - 1;
  return SHM_DIR;
}
