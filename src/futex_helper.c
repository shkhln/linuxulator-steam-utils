#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/futex.h>

#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>

int main(int argc, char* argv[]) {

  if (argc != 4) {
    exit(EXIT_FAILURE);
  }

  intmax_t offset = strtoimax(argv[1], NULL, 10);
  assert(errno != ERANGE && errno != EINVAL);

  intmax_t map_fd = strtoimax(argv[2], NULL, 10);
  assert(errno != ERANGE && errno != EINVAL);

  intmax_t out_fd = strtoimax(argv[3], NULL, 10);
  assert(errno != ERANGE && errno != EINVAL);

  void* base = mmap(NULL, offset + 4, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
  if (base == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  uintptr_t uaddr = (uintptr_t)base + offset;

  int err = syscall(SYS_futex, uaddr, FUTEX_WAKE, 1, NULL, NULL, 0);
  //~ fprintf(stderr, "futex(_, FUTEX_WAKE, ...) -> %d\n", err);

  int nbytes = write(out_fd, &err, sizeof(err));
  if (nbytes == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  close(out_fd);

  return EXIT_SUCCESS;
}
