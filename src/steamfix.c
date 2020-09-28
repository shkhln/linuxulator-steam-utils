#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Do not allow Breakpad to override libSegFault handlers */

#include <execinfo.h>
#include <signal.h>

static int (*libc_sigaction)(int, const struct sigaction*, struct sigaction*) = NULL;

int sigaction(int sig, const struct sigaction* restrict act, struct sigaction* restrict oact) {

  if (!libc_sigaction) {
    libc_sigaction = dlsym(RTLD_NEXT, "sigaction");
  }

  void* buffer[2];
  int nframes = backtrace(buffer, 2);
  assert(nframes == 2);

  char* caller_str = *backtrace_symbols(buffer + 1, 1);
  assert(caller_str != NULL);

  char* p = strrchr(caller_str, '/');
  if (p && strncmp("crashhandler.so", p + 1, sizeof("crashhandler.so") - 1) == 0) {
    return EINVAL;
  }

  return libc_sigaction(sig, act, oact);
}

/* Let's rewrite a few commands as well... */

static int (*libc_system)(const char*) = NULL;

#define SYSTEM_ENV   "LD_LIBRARY_PATH=\"$SYSTEM_LD_LIBRARY_PATH\" PATH=\"$SYSTEM_PATH\""
#define XDG_OPEN_CMD "'/usr/bin/xdg-open'"

int system(const char* command) {

  if (!libc_system) {
    libc_system = dlsym(RTLD_NEXT, "system");
  }

  /* such as xdg-open */

  if (strncmp(command, SYSTEM_ENV, sizeof(SYSTEM_ENV) - 1) == 0 && (
        strncmp(command + sizeof(SYSTEM_ENV) - 1, " "  XDG_OPEN_CMD " ", sizeof(" "  XDG_OPEN_CMD " ") - 1) == 0 ||
        strncmp(command + sizeof(SYSTEM_ENV) - 1, "  " XDG_OPEN_CMD " ", sizeof("  " XDG_OPEN_CMD " ") - 1) == 0)
  ) {

    const char* xdg_open_args = strstr(command, XDG_OPEN_CMD " ") + sizeof(XDG_OPEN_CMD " ") - 1;

    char* format_str = "LD_LIBRARY_PATH='' LD_PRELOAD='' PATH=${FREEBSD_PATH} xdg-open %s";

    int   buf_len = strlen(format_str) + strlen(xdg_open_args) + 1;
    char* buf     = malloc(buf_len);

    snprintf(buf, buf_len, format_str, xdg_open_args);

    int err = libc_system(buf);
    free(buf);

    return err;
  }

  /* or steamwebhelper */

  if (strstr(command, "steamwebhelper.sh")) {

    char* browser_env = getenv("LSU_BROWSER");
    if (!browser_env || strcmp(browser_env, "1") == 0) {

      char* format_str =
        "LD_PRELOAD=webfix.so"
        " '%s.patched' %s"
        " --no-sandbox"
        " --no-zygote"
        " --in-process-gpu" // YMMV
        //" --enable-logging=stderr"
        //" --v=0"
      ;

      int   buf_len = strlen(format_str) + strlen(command) + 1;
      char* buf     = malloc(buf_len);

      char* webhelper_path = strdup(command + 1);
      char* webhelper_args = strstr(webhelper_path, "steamwebhelper.sh' ") + sizeof("steamwebhelper.sh' ") - 1;
      assert(webhelper_args[-2] == '\'');
      webhelper_args[-2] = '\0';

      snprintf(buf, buf_len, format_str, webhelper_path, webhelper_args);
      free(webhelper_path);

      fprintf(stderr, "[[%s]]\n", buf);

      int err = libc_system(buf);
      free(buf);

      return err;

    } else {
      return 1;
    }
  }

  return libc_system(command);
}

/* Correct uname because for whatever reason Linuxulator reports "i686" to 32-bit applications on amd64 */

#include <sys/utsname.h>

static int (*libc_uname)(struct utsname*) = NULL;

int uname(struct utsname* name) {

  if (!libc_uname) {
    libc_uname = dlsym(RTLD_NEXT, "uname");
  }

  int err = libc_uname(name);
  if (err == 0 && access("/usr/lib64", F_OK) == 0) {
    snprintf(name->machine, _UTSNAME_MACHINE_LENGTH, "x86_64");
  }

  return err;
}

/* Handle Steam restart */

#include <stdbool.h>
#include <linux/limits.h>

static int    program_argc;
static char** program_argv;
static char   program_path[PATH_MAX];

__attribute__((constructor))
static void restart_fix_init(int argc, char** argv, char** env) {

  program_argc = argc;
  program_argv = argv;

  ssize_t nchars = readlink("/proc/self/exe", program_path, sizeof(program_path));
  assert(nchars > 0);
  program_path[nchars] = '\0';
}

extern char** environ;

static bool drop_urls_on_restart = false;

static __attribute__((__noreturn__)) void restart() {

  printf("Restarting Steam...\n");

  char pidfile_path[PATH_MAX];
  snprintf(pidfile_path, sizeof(pidfile_path), "%s/%s", getenv("HOME"), ".steam/steam.pid");

  unlink(pidfile_path);

  if (drop_urls_on_restart) {

    char** argv = malloc(sizeof(char*) * (program_argc + 1));

    int j = 0;
    for (int i = 0; i < program_argc; i++) {
      char* arg = program_argv[i];
      if (strncmp(arg, "steam://", sizeof("steam://") - 1) != 0) {
        argv[j] = arg; j++;
      }
    }
    argv[j] = NULL;

    execve(program_path, argv, environ);

  } else {
    execve(program_path, program_argv, environ);
  }

  perror("execve");
  abort();
}

static __attribute__((__noreturn__)) void (*libc_exit)(int) = NULL;

void exit(int status) {

  if (status == 42) {

    if (system("patch-steam") != 0 || system("upgrade-steam-runtime") != 0) {
      libc_exit(EXIT_FAILURE);
    }

    restart();

  } else {

    if (!libc_exit) {
      libc_exit = dlsym(RTLD_NEXT, "exit");
    }

    libc_exit(status);
  }
}

static int (*libc_fputs)(const char*, FILE*) = NULL;

int fputs(const char* str, FILE* stream) {

  if (!libc_fputs) {
    libc_fputs = dlsym(RTLD_NEXT, "fputs");
  }

  if (stream == stderr && strncmp(str, "ExecuteSteamURL:", sizeof("ExecuteSteamURL:") - 1) == 0) {
    drop_urls_on_restart = true;
  }

  return libc_fputs(str, stream);
}

/* We need to redirect those futex syscalls */

static void* (*libc_dlmopen)(Lmid_t, const char*, int) = NULL;

void* dlmopen(Lmid_t lmid, const char* path, int mode) {

  if (!libc_dlmopen) {
    libc_dlmopen = dlsym(RTLD_NEXT, "dlmopen");
  }

  void* p = NULL;

  if (strstr(path, "chromehtml.so") != NULL) {

    char* format_str = "%s.patched";

    int   buf_len = strlen(format_str) + strlen(path) + 1;
    char* buf     = malloc(buf_len);

    snprintf(buf, buf_len, format_str, path);

    p = libc_dlmopen(lmid, buf, mode);

    free(buf);

  } else {
    p = libc_dlmopen(lmid, path, mode);
  }

  return p;
}

/* Apparently communication with CSGO process breaks when Steam receives an unexpected EAGAIN error, which prevents the game from starting */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

static ssize_t (*libc_send)(int s, const void* msg, size_t len, int flags) = NULL;

ssize_t send(int s, const void* msg, size_t len, int flags) {

  if (!libc_send) {
    libc_send = dlsym(RTLD_NEXT, "send");
  }

  //~ fprintf(stderr, "%s(%d, %p, %d, %d)\n", __func__, s, msg, len, flags);

  int sleeped_ms = 0;

  ssize_t nbytes = -1;
  while (1) {
    nbytes = libc_send(s, msg, len, flags);
    if (nbytes == -1 && errno == EAGAIN && !(flags & MSG_DONTWAIT) && !(fcntl(s, F_GETFL) & O_NONBLOCK)) {
      if (sleeped_ms > 50) {
        break;
      }
      usleep(5000);
      sleeped_ms += 5;
    } else {
      break;
    }
  }

  //~ fprintf(stderr, "%s -> %d\n", __func__, nbytes);
  //~ if (nbytes == -1) {
    //~ perror(__func__);
  //~ }

  return nbytes;
}

/* Steam doesn't have the ability to detach anything, but this action must succeed, otherwise Steam won't see any devices */

int libusb_detach_kernel_driver(void* dev, int interface) {
  return 0;
}
