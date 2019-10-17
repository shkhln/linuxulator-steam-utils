#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

// PCHECK(0 == fstat_ret) @ https://cs.chromium.org/chromium/src/sandbox/linux/services/thread_helpers.cc?l=41&rcl=90c6c958243e775074d81e19c204f196d8e76990
// CHECK_LE(3UL, task_stat.st_nlink) @ https://cs.chromium.org/chromium/src/sandbox/linux/services/thread_helpers.cc?l=44&rcl=90c6c958243e775074d81e19c204f196d8e76990

#include <sys/stat.h>

static int (*libc_fxstatat64)(int, int, const char*, struct stat64*, int) = NULL;

int __fxstatat64(int ver, int dirfd, const char* path, struct stat64* stat_buf, int flags) {

  if (!libc_fxstatat64) {
    libc_fxstatat64 = dlsym(RTLD_NEXT, "__fxstatat64");
  }

  int err = -1;

  if (strcmp(path, "self/task/") == 0) {
    stat_buf->st_nlink = 3;
    err = 0;
  } else {
    err = libc_fxstatat64(ver, dirfd, path, stat_buf, flags);
  }

  return err;
}

// CHECK(clock_gettime(clk_id, &ts) == 0) @ https://cs.chromium.org/chromium/src/base/time/time_now_posix.cc?l=52&rcl=b6ad4da425c33a31e4e08b67ce070a0c52082358

#include <time.h>

static int (*libc_clock_gettime)(clockid_t clock_id, struct timespec* tp) = NULL;

int clock_gettime(clockid_t clock_id, struct timespec* tp) {

  if (!libc_clock_gettime) {
    libc_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
  }

  int err = -1;

  // https://github.com/freebsd/freebsd/blob/14aef6dfca96006e52b8fb920bde7c612ba58b79/sys/compat/linux/linux_time.c#L192
  switch (clock_id) {
    case CLOCK_THREAD_CPUTIME_ID:
      err = libc_clock_gettime(-2, tp);
      break;
    case CLOCK_PROCESS_CPUTIME_ID:
      err = libc_clock_gettime(-6, tp);
      break;
    default:
      err = libc_clock_gettime(clock_id, tp);
  }

  /*if (err == -1) {
    fprintf(stderr, "%s(%d, %p) -> %d [%s]\n", __func__, clock_id, tp, err, strerror(errno));
  }*/

  return err;
}

/* This really should not be necessary with disabled sandbox... */

#include <sys/types.h>
#include <sys/socket.h>

static int (*libc_setsockopt)(int, int, int, const void*, socklen_t) = NULL;

int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {

  if (!libc_setsockopt) {
    libc_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
  }

  if (optname == SO_PASSCRED) {
    return 0;
  }

  return libc_setsockopt(s, level, optname, optval, optlen);
}
