#define _GNU_SOURCE

#ifndef SKIP_EPOLLONESHOT_WORKAROUND

/* Work around timeout on login issue */

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/epoll.h>

static int (*libc_epoll_ctl)(int, int, int, struct epoll_event*) = NULL;

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {

  if (!libc_epoll_ctl) {
    libc_epoll_ctl = dlsym(RTLD_NEXT, "epoll_ctl");
  }

  if (event) {
    event->events |= EPOLLERR;
  }

  int err = libc_epoll_ctl(epfd, op, fd, event);

  if (err == -1 && op == EPOLL_CTL_MOD && errno == ENOENT) {
    errno = 0;
    err   = libc_epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
  }

  if (err == -1) {
    perror(__func__);
  }

  return err;
}

#endif
