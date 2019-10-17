#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/futex.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef __i386__
#define FUTEX_HELPER "fhelper64"
#endif

#ifdef __x86_64__
#define FUTEX_HELPER "fhelper32"
#endif

#define RETRY_LIMIT 5

extern char* program_invocation_short_name;

#ifndef SKIP_FUTEX_WORKAROUND

static SLIST_HEAD(slisthead, mpfd_entry) mapped_fds = SLIST_HEAD_INITIALIZER(mapped_fds);

struct mpfd_entry {
  SLIST_ENTRY(mpfd_entry) entries;
  int       fd;
  uintptr_t addr;
  size_t    len;
  off_t     offset;
};

__attribute__((constructor))
static void mapped_fds_list_init() {
  SLIST_INIT(&mapped_fds);
}

static pthread_mutex_t mapped_fds_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool find_fd_and_offset(uintptr_t addr, int* fd, int* offset) {

  bool found = false;

  pthread_mutex_lock(&mapped_fds_mutex);

  struct mpfd_entry* entry = NULL;
  SLIST_FOREACH(entry, &mapped_fds, entries) {

    if (entry->addr <= addr && addr < entry->addr + entry->len) {

      found = true;

      assert(entry->offset == 0);

      *fd     = entry->fd;
      *offset = addr - entry->addr;

      break;
    }
  }

  pthread_mutex_unlock(&mapped_fds_mutex);

  return found;
}

static int wake(uintptr_t addr, int timeout_ms) {

  int fd, offset;

  if (find_fd_and_offset(addr, &fd, &offset)) {

    int inout[2];
    {
      int err = pipe2(inout, O_NONBLOCK);
      assert(err == 0);
    }

    pid_t pid = 0;

    char offset_str[10];
    snprintf(offset_str, sizeof(offset_str), "%d", offset);

    char map_fd_str[10];
    snprintf(map_fd_str, sizeof(map_fd_str), "%d", fd);

    char out_fd_str[10];
    snprintf(out_fd_str, sizeof(out_fd_str), "%d", inout[1]);

    char* const args[] = {
      FUTEX_HELPER,
      offset_str,
      map_fd_str,
      out_fd_str,
      NULL
    };

    int err = posix_spawnp(&pid, FUTEX_HELPER, NULL, NULL, args, NULL);
    if (err == 0) {

      int futex_err  = 0;
      int sleeped_ms = 0;

      while (read(inout[0], &futex_err, sizeof(int)) == -1) {

        if (errno == EAGAIN) {

          if (sleeped_ms > timeout_ms) {
            fprintf(stderr, "%s: " FUTEX_HELPER " timeout\n", program_invocation_short_name);
            futex_err = -1;
            break;
          }

          usleep(5000);
          sleeped_ms += 5;

        } else {
          perror("read");
          break;
        }
      }

      close(inout[0]);
      close(inout[1]);

      int status = 0;
      int werr = waitpid(pid, &status, 0);
      if (werr != -1) {

        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
          fprintf(stderr, FUTEX_HELPER " exited with code %d\n", WEXITSTATUS(status));
        }

      } else {
        //perror("waitpid");
      }

      return futex_err;

    } else {
      perror("posix_spawnp");
    }

  } else {
    assert(0);
  }

  return 0;
}

#endif

long int llacsys(long int number, ...) {

  if (number == SYS_get_robust_list) {

    va_list args;
    va_start(args, number);

    int pid                            = va_arg(args, int);
    struct robust_list_head** head_ptr = va_arg(args, struct robust_list_head**);
    size_t* len_ptr                    = va_arg(args, size_t *);

    va_end(args);

    return syscall(SYS_get_robust_list, pid, head_ptr, len_ptr);
  }

  if (number == SYS_gettid) {
    return syscall(SYS_gettid);
  }

  if (number == SYS_futex) {

    va_list args;
    va_start(args, number);

    int* uaddr                     = va_arg(args, int*);
    int  futex_op                  = va_arg(args, int);
    int  val                       = va_arg(args, int);
    const struct timespec* timeout = va_arg(args, struct timespec*);
    int* uaddr2                    = va_arg(args, int*);
    int  val3                      = va_arg(args, int);

    va_end(args);

    int err = syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);

#ifndef SKIP_FUTEX_WORKAROUND

    if (err == 0 && futex_op == FUTEX_WAKE) {

      for (int i = 0; i < RETRY_LIMIT; i++) {
        err = wake((uintptr_t)uaddr, 250 * (int)pow(2, i));
        if (err != -1) {
          return err;
        }
      }

      fprintf(stderr, "%s: " FUTEX_HELPER " failed\n", program_invocation_short_name);
      return 0;

    } else {
      return err;
    }

#else
    return err;
#endif
  }

  assert(0);
}

#ifndef SKIP_FUTEX_WORKAROUND

static void* (*libc_mmap)(void*, size_t, int, int, int, off_t) = NULL;

void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {

  if (!libc_mmap) {
    libc_mmap = dlsym(RTLD_NEXT, "mmap");
  }

  void* p = libc_mmap(addr, len, prot, flags, fd, offset);

  if (fd != -1 && p != MAP_FAILED) {

    struct mpfd_entry* entry = malloc(sizeof(struct mpfd_entry));
    entry->fd     = fd;
    entry->addr   = (uintptr_t)p;
    entry->len    = len;
    entry->offset = offset;

    pthread_mutex_lock(&mapped_fds_mutex);

    SLIST_INSERT_HEAD(&mapped_fds, entry, entries);

    pthread_mutex_unlock(&mapped_fds_mutex);
  }

  return p;
}

static int (*libc_munmap)(void*, size_t) = NULL;

int munmap(void* addr, size_t len) {

  if (!libc_munmap) {
    libc_munmap = dlsym(RTLD_NEXT, "munmap");
  }

  pthread_mutex_lock(&mapped_fds_mutex);

  struct mpfd_entry* entry = NULL;
  SLIST_FOREACH(entry, &mapped_fds, entries) {

    if (entry->addr == (uintptr_t)addr) {

      assert(entry->len == len);

      SLIST_REMOVE(&mapped_fds, entry, mpfd_entry, entries);
      free(entry);

      break;
    }
  }

  pthread_mutex_unlock(&mapped_fds_mutex);

  return libc_munmap(addr, len);
}

#endif
