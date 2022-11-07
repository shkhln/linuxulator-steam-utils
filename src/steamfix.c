#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
  Prevent LD_PRELOAD="steamfix.so:libSegFault.so" from leaking into spawned processes
  (largely to avoid those "object cannot be preloaded" ld.so complaints)
*/

static char* ld_preload = NULL;

__attribute__((constructor))
static void clear_ld_preload() {

  char* keep = getenv("LSU_KEEP_LD_PRELOAD");
  if (keep && strcmp(keep, "1") == 0)
    return;

  char* s = getenv("LD_PRELOAD");
  if (s) {
    ld_preload = strdup(s);
    unsetenv("LD_PRELOAD");
  }
}

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

// ubuntu12_32/reaper depends on unimplemented PR_SET_CHILD_SUBREAPER
void purge_reaper(const char* command) {
  char* reaper_str = strstr(command, "/reaper SteamLaunch");
  if (reaper_str != NULL) {

    char* e = strstr(reaper_str, "--");
    assert(e != NULL);

    for (char* p = reaper_str; p >= command && *p != ' '; p--) {
      *p = ' ';
    }

    for (char* p = reaper_str; p <= e + 1; p++) {
      *p = ' ';
    }
  }
}

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
#if __FreeBSD_version < 1300139
        "LD_PRELOAD=webfix.so"
#endif
        " '%s' %s"
        " --no-sandbox"
#if __FreeBSD_version < 1300139
        " --no-zygote"
#endif
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

  purge_reaper(command);

  return libc_system(command);
}

static int (*libc_execvp)(const char*, char* const []) = NULL;

int execvp(const char* path, char* const argv[]) {

  if (!libc_execvp) {
    libc_execvp = dlsym(RTLD_NEXT, "execvp");
  }

  for (int i = 0; argv[i] != NULL; i++) {
    purge_reaper(argv[i]);
  }

  return libc_execvp(path, argv);
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

  if (ld_preload) {
    setenv("LD_PRELOAD", ld_preload, 1);
  }

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

/* Apparently communication with CSGO process breaks when Steam receives an unexpected EAGAIN error, which prevents the game from starting */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

static ssize_t (*libc_send)(int s, const void* msg, size_t len, int flags) = NULL;

ssize_t send(int s, const void* msg, size_t len, int flags) {

  if (!libc_send) {
    libc_send = dlsym(RTLD_NEXT, "send");
  }

  ssize_t nbytes = libc_send(s, msg, len, flags);

  if (nbytes == -1 && errno == EAGAIN && flags == MSG_NOSIGNAL && !(fcntl(s, F_GETFL) & O_NONBLOCK)) {
    nbytes = libc_send(s, msg, len, 0x00020000);
  }

  return nbytes;
}

/* Steam doesn't have the ability to detach anything, but this action must succeed, otherwise Steam won't see any devices */

int libusb_detach_kernel_driver(void* dev, int interface) {
  return 0;
}

/* Steam repeatedly tries to set SO_SNDBUF to 0 (what for?), spamming the console with warnings when this doesn't work */

static int (*libc_setsockopt)(int s, int level, int optname, const void *optval, socklen_t optlen) = NULL;

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) {

  if (!libc_setsockopt) {
    libc_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
  }

  if (level == SOL_SOCKET && optname == SO_SNDBUF && optlen == sizeof(int) && optval && *(int*)optval == 0) {
    return 0;
  }

  return libc_setsockopt(s, level, optname, optval, optlen);
}

// https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=267616

#include <stdarg.h>
#include <linux/futex.h>
#include <sys/syscall.h>

static void* (*libc_dlmopen)(Lmid_t, const char*, int) = NULL;

void* dlmopen(Lmid_t lmid, const char* path, int mode) {

  if (!libc_dlmopen) {
    libc_dlmopen = dlsym(RTLD_NEXT, "dlmopen");
  }

  void* p = NULL;

  if (strstr(path, "chromehtml.so") != NULL || strstr(path, "steamclient.so") != NULL || strstr(path, "steamui.so") != NULL) {

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

long int llacsys(long int number, ...) {

  if (number == SYS_get_robust_list) {

    va_list args;
    va_start(args, number);

    int pid                            = va_arg(args, int);
    struct robust_list_head** head_ptr = va_arg(args, struct robust_list_head**);
    size_t* len_ptr                    = va_arg(args, size_t *);

    va_end(args);

    int err = syscall(SYS_get_robust_list, pid, head_ptr, len_ptr);
    if (err == 0) {
      *len_ptr = 12;
    }

    return err;
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

    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
  }

  assert(0);
}
