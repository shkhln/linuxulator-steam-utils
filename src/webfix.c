#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// show CEF version

static int (*orig_cef_initialize)(void*, void*, void*, void*) = NULL;

int cef_initialize(void* args, void* settings, void* application, void* windows_sandbox_info) {

  if (!orig_cef_initialize) {
    orig_cef_initialize = dlsym(RTLD_NEXT, "cef_initialize");
    assert(orig_cef_initialize != NULL);
  }

  int (*cef_version_info)(int) = dlsym(RTLD_NEXT, "cef_version_info");
  assert(cef_version_info != NULL);

  int cef_major    = cef_version_info(0);
  int cef_minor    = cef_version_info(1);
  int cef_patch    = cef_version_info(2);
  int cef_commit   = cef_version_info(3);
  int chrome_major = cef_version_info(4);
  int chrome_minor = cef_version_info(5);
  int chrome_build = cef_version_info(6);
  int chrome_patch = cef_version_info(7);

  fprintf(stderr, "[[CEF version = %d.%d.%d.%d, Chrome version = %d.%d.%d.%d]]\n",
    cef_major, cef_minor, cef_patch, cef_commit, chrome_major, chrome_minor, chrome_build,chrome_patch);

  return orig_cef_initialize(args, settings, application, windows_sandbox_info);
}

#ifndef SKIP_PROC_SELF_TASK_WORKAROUND

// PCHECK(0 == fstat_ret) @ https://cs.chromium.org/chromium/src/sandbox/linux/services/thread_helpers.cc?l=41&rcl=90c6c958243e775074d81e19c204f196d8e76990
// CHECK_LE(3UL, task_stat.st_nlink) @ https://cs.chromium.org/chromium/src/sandbox/linux/services/thread_helpers.cc?l=44&rcl=90c6c958243e775074d81e19c204f196d8e76990

#include <ctype.h>
#include <sys/stat.h>

static int (*libc_close)     (int fd)                                     = NULL;
static int (*libc_fxstat64)  (int, int, struct stat64*)                   = NULL;
static int (*libc_fxstatat64)(int, int, const char*, struct stat64*, int) = NULL;
static int (*libc_open64)    (const char*, int, ...)                      = NULL;

static int proc_self_task_fd = -1;

__attribute__((constructor))
static void proc_self_task_workaround_init() {

  libc_fxstat64     = dlsym(RTLD_NEXT, "__fxstat64");
  libc_fxstatat64   = dlsym(RTLD_NEXT, "__fxstatat64");
  libc_close        = dlsym(RTLD_NEXT, "close");
  libc_open64       = dlsym(RTLD_NEXT, "open64");

  proc_self_task_fd = open("/dev/null", O_RDONLY);
}

int __fxstatat64(int ver, int dirfd, const char* path, struct stat64* stat_buf, int flags) {

  if (dirfd == proc_self_task_fd) {
    assert(isdigit(path[0]));
    errno = ENOENT;
    return -1;
  }

  if (strcmp(path, "self/task/") == 0) {
    stat_buf->st_nlink = 1 + 2;
    return 0;
  }

  return libc_fxstatat64(ver, dirfd, path, stat_buf, flags);
}

// thread_helpers.cc before https://codereview.chromium.org/914053002

int __fxstat64(int ver, int dirfd, struct stat64* stat_buf) {

  if (dirfd == proc_self_task_fd) {
    stat_buf->st_nlink = 1 + 2;
    return 0;
  }

  return libc_fxstat64(ver, dirfd, stat_buf);
}

int open64(const char* path, int flags, ...) {

  if (strcmp(path, "/proc/self/task/") == 0) {
    return proc_self_task_fd;
  }

  mode_t mode = 0;

  if (flags & O_CREAT) {

    va_list args;
    va_start(args, flags);

    mode = va_arg(args, int);

    va_end(args);
  }

  return open(path, flags, mode);
}

int close(int fd) {

  // close <- libcef.so`calloc <- libdl.so.2
  if (!libc_close) {
    return 0;
  }

  if (fd == proc_self_task_fd) {
    return 0;
  }

  return libc_close(fd);
}

#endif

#ifndef SKIP_CLOCK_GETTIME_WORKAROUND

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
    case CLOCK_MONOTONIC_RAW:
      err = libc_clock_gettime(CLOCK_MONOTONIC, tp); // ?
      break;
    default:
      err = libc_clock_gettime(clock_id, tp);
  }

  /*if (err == -1) {
    fprintf(stderr, "%s(%d, %p) -> %d [%s]\n", __func__, clock_id, tp, err, strerror(errno));
  }*/

  return err;
}

#endif

/* SO_PASSCRED workaround */

#include <execinfo.h>
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

static struct msghdr* copy_and_add_credentials(struct msghdr* msg) {

  struct msghdr* result_msg = malloc(sizeof(struct msghdr));
  memcpy(result_msg, msg, sizeof(struct msghdr));

  int   result_cmsg_len = msg->msg_controllen + CMSG_SPACE(sizeof(struct ucred));
  char* result_cmsg     = malloc(result_cmsg_len);

  memset(result_cmsg, 0, result_cmsg_len);
  memcpy(result_cmsg, msg->msg_control, msg->msg_controllen);

  result_msg->msg_control    = result_cmsg;
  result_msg->msg_controllen = result_cmsg_len;

  struct cmsghdr* cmsg_in  = NULL;
  struct cmsghdr* cmsg_out = NULL;
  while (true) {
    if (!cmsg_in) {
      cmsg_in  = CMSG_FIRSTHDR(msg);
      cmsg_out = CMSG_FIRSTHDR(result_msg);
    } else {
      cmsg_in  = CMSG_NXTHDR(msg, cmsg_in);
      cmsg_out = CMSG_NXTHDR(result_msg, cmsg_out);
    }

    if (cmsg_in != NULL) {
      memcpy(cmsg_out, cmsg_in, cmsg_in->cmsg_len);
    } else {

      cmsg_out->cmsg_len   = CMSG_LEN(sizeof(struct ucred));
      cmsg_out->cmsg_level = SOL_SOCKET;
      cmsg_out->cmsg_type  = SCM_CREDENTIALS;

      struct ucred* ucred = (struct ucred*)CMSG_DATA(cmsg_out);

      ucred->pid = getpid();
      ucred->uid = getuid();
      ucred->gid = getgid();

      break;
    }
  }

  return result_msg;
}

static ssize_t (*libc_sendmsg)(int s, const struct msghdr* msg, int flags) = NULL;

ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {

  if (!libc_sendmsg) {
    libc_sendmsg = dlsym(RTLD_NEXT, "sendmsg");
  }

  ssize_t nbytes = -1;

  void* buffer[2];
  int nframes = backtrace(buffer, 2);
  assert(nframes == 2);

  char* caller_str = *backtrace_symbols(buffer + 1, 1);
  assert(caller_str != NULL);

  char* p = strrchr(caller_str, '/');

  if (p && strncmp("libcef.so", p + 1, sizeof("libcef.so") - 1) == 0) {
    struct msghdr* xmsg = copy_and_add_credentials((struct msghdr*)msg);
    nbytes = libc_sendmsg(s, xmsg, flags);
    free(xmsg->msg_control);
    free(xmsg);
  } else {
    nbytes = libc_sendmsg(s, msg, flags);
  }

  return nbytes;
}
