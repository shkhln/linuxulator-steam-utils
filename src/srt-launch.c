#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/nv.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LSU_LAUNCHER_SOCKET "/lsu-launcher-socket"

int main(int argc, char* argv[], char* envp[]) {

  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    err(1, "socket");
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strlcpy(addr.sun_path, LSU_LAUNCHER_SOCKET, sizeof(addr.sun_path));

  if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    err(1, "can't connect to " LSU_LAUNCHER_SOCKET);
  }

  nvlist_t* request = nvlist_create(0);

  bool alongside_steam = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--") == 0) {
      i++;
      for (; i < argc; i++) {
        nvlist_append_string_array(request, "args", argv[i]);
      }
    } else if (strcmp(argv[i], "--alongside-steam") == 0) {
      alongside_steam = true;
    } else if (strncmp(argv[i], "--forward-fd=", sizeof("--forward-fd=") - 1) == 0) {
      errno = 0;
      int fd = (int)strtoumax(argv[i] + sizeof("--forward-fd=") - 1, NULL, 10);
      if (errno == 0) {
        assert(fcntl(fd, F_GETFD) == 0);
        nvlist_append_descriptor_array(request, "forward-fd", fd);
        nvlist_append_number_array(request, "forward-fd-orig", fd);
      } else {
        warn("ignoring arg %d: %s", i, argv[i]);
      }
    } else {
      warnx("ignoring arg %d: %s", i, argv[i]);
    }
  }

  assert(alongside_steam);

  nvlist_add_descriptor(request, "fd0", 0);
  nvlist_add_descriptor(request, "fd1", 1);
  nvlist_add_descriptor(request, "fd2", 2);

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    nvlist_add_string(request, "cwd", cwd);
  }

  int envc = 0;
  for (; envp[envc] != NULL; envc++);

  nvlist_add_string_array(request, "env", (const char* const*)envp, envc);

  if (nvlist_send(sock_fd, request) == -1) {
    err(1, "nvlist_send() failed");
  }

  nvlist_t* response = nvlist_recv(sock_fd, 0);

  return nvlist_get_number(response, "exit-code");
}
