#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/epoll.h>

#define EPOLLEXCLUSIVE (1 << 28)

static int (*libc_epoll_ctl)(int, int, int, struct epoll_event*) = NULL;

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {

  static int warning_printed = 0;

  if (!libc_epoll_ctl) {
    libc_epoll_ctl = dlsym(RTLD_NEXT, "epoll_ctl");
    assert(libc_epoll_ctl != epoll_ctl);
    assert(libc_epoll_ctl != NULL);
  }

  if (event != NULL && event->events & EPOLLEXCLUSIVE) {
    if (!__atomic_test_and_set(&warning_printed, __ATOMIC_SEQ_CST)) {
      fprintf(stderr, "%s: no EPOLLEXCLUSIVE for you\n", __func__);
    }
    event->events &= ~EPOLLEXCLUSIVE;
  }

  return libc_epoll_ctl(epfd, op, fd, event);
}
