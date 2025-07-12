#define _GNU_SOURCE

#include <sys/socket.h>

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
  Prevent LD_PRELOAD="steamfix.so:libSegFault.so" from leaking into spawned processes
  (largely to avoid those "object cannot be preloaded" ld.so complaints)
*/

__attribute__((constructor))
static void clear_ld_preload() {

  char* keep = getenv("LSU_KEEP_LD_PRELOAD");
  if (keep && strcmp(keep, "1") == 0) {
    return;
  }

  if (getenv("LD_PRELOAD") != NULL) {
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

char* modify_webhelper_command(const char* command) {

  char* format_str =
#if 0
    "LD_PRELOAD=webfix.so "
#endif
    "%s.patched' %s"
    " --no-sandbox"
    //" --enable-logging=stderr"
    //" --v=0"
  ;

  int   buf_len = strlen(format_str) + strlen(command) + 1;
  char* buf     = malloc(buf_len);

  char* webhelper_path = strdup(command);
  char* webhelper_args = strstr(webhelper_path, "steamwebhelper.sh' ") + sizeof("steamwebhelper.sh' ") - 1;
  assert(webhelper_args[-2] == '\'');
  webhelper_args[-2] = '\0';

  snprintf(buf, buf_len, format_str, webhelper_path, webhelper_args);
  free(webhelper_path);

  return buf;
}

static int (*libc_system)(const char*) = NULL;

#define XDG_OPEN_CMD "'/usr/bin/xdg-open'"

int system(const char* command) {

  if (!libc_system) {
    libc_system = dlsym(RTLD_NEXT, "system");
  }

  /* such as xdg-open */

  if (strstr(command, XDG_OPEN_CMD)) {

    char* buf = strdup(command);

    char* xdg_open_cmd = strstr(buf, XDG_OPEN_CMD);
    memset(xdg_open_cmd, ' ', sizeof(XDG_OPEN_CMD) - 1);
    memcpy(xdg_open_cmd, "xdg-open", sizeof("xdg-open") - 1);

    fprintf(stderr, "[[%s]]\n", buf);

    int err = libc_system(buf);
    free(buf);

    return err;
  }

  /* or steamwebhelper */

  if (strstr(command, "steamwebhelper.sh")) {

    char* browser_env = getenv("LSU_BROWSER");
    if (!browser_env || strcmp(browser_env, "1") == 0) {

      char* buf = modify_webhelper_command(command);
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

static int (*libc_execvpe)(const char* file, char* const argv[], char* const envp[]) = NULL;

int execvpe(const char* file, char* const argv[], char* const envp[]) {

  if (!libc_execvpe) {
    libc_execvpe = dlsym(RTLD_NEXT, "execvpe");
  }

  for (int i = 0; argv[i] != NULL; i++) {
    purge_reaper(argv[i]);
  }

  return libc_execvpe(file, argv, envp);
}

static int (*libc_execv)(const char*, char* const []) = NULL;

int execv(const char* file, char* const argv[]) {

  if (!libc_execv) {
    libc_execv = dlsym(RTLD_NEXT, "execv");
  }

  int arg_count = 0;
  int webhelper_str_index = -1;

  for (int i = 0; argv[arg_count] != NULL; i++, arg_count = i) {
    if (strstr(argv[i], "steamwebhelper.sh") && !strstr(argv[i], "steamwebhelper.sh.patched")) {
      webhelper_str_index = i;
    }
  }

  if (webhelper_str_index != -1) {

    size_t argv_size_in_bytes = sizeof(char*) * (arg_count + 1);
    char** argv2 = malloc(argv_size_in_bytes);
    memcpy(argv2, argv, argv_size_in_bytes);

    argv2[webhelper_str_index] = modify_webhelper_command(argv[webhelper_str_index]);

    for (int i = 0; argv2[i] != NULL; i++) {
      fprintf(stderr, "[[%s]]\n", argv2[i]);
    }

    int err = libc_execv(file, argv2);
    perror("execv");

    free(argv2[webhelper_str_index]);
    free(argv2);

    return err;

  } else {
    return libc_execv(file, argv);
  }
}

/* Let's redirect Steam to our lsof stub */

static FILE* (*libc_popen)(const char*, const char*) = NULL;

FILE* popen(const char* command, const char* type) {

  if (!libc_popen) {
    libc_popen = dlsym(RTLD_NEXT, "popen");
  }

  if (strncmp(command, "/usr/sbin/lsof", sizeof("/usr/sbin/lsof") - 1) == 0) {
    return libc_popen(&command[sizeof("/usr/sbin/") - 1], type);
  } else {
    return libc_popen(command, type);
  }
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

/* ye olde werkshoppe amendement */

#if __FreeBSD_version < 1400097

#include <sys/sendfile.h>
#include <sys/syscall.h>

static ssize_t (*libc_sendfile64)(int out_fd, int in_fd, off64_t* offset, size_t count) = NULL;

ssize_t sendfile64(int out_fd, int in_fd, off64_t* offset, size_t count) {

  if (!libc_sendfile64) {
    libc_sendfile64 = dlsym(RTLD_NEXT, "sendfile64");
  }

  //~ fprintf(stderr, "%s(%d, %d, %p (%lld), %d)\n", __func__, out_fd, in_fd, offset, offset != NULL ? *offset : 0, count);

  int nbytes = libc_sendfile64(out_fd, in_fd, offset, count);
  if (nbytes == -1 && errno == EINVAL && offset == NULL) {

    off64_t in_offset = lseek(in_fd, 0, SEEK_CUR);
    assert(in_offset != -1);

    off64_t out_offset = lseek(out_fd, 0, SEEK_CUR);
    assert(out_offset != -1);

    //~ fprintf(stderr, "%s: fallback to copy_file_range(%d, %p (%lld), %d, %p (%lld), %d, 0)\n",
      //~ __func__, in_fd, &in_offset, in_offset, out_fd, &out_offset, out_offset, count);

    nbytes = syscall(SYS_copy_file_range, in_fd, &in_offset, out_fd, &out_offset, count, 0);
    if (nbytes != -1) {
      lseek(in_fd, in_offset + nbytes, SEEK_SET);
    }
  }

  //~ fprintf(stderr, "%s -> %d (errno = %d)\n", __func__, nbytes, errno);
  return nbytes;
}

#endif // __FreeBSD_version < 1400097

/* Steam's "add a new library folder" logic is dumber than an average /r/freebsd poster,
 so we have to sanitize things a bit. */

#include <mntent.h>

struct mntent* (*libc_getmntent)(FILE*) = NULL;

struct mntent* getmntent(FILE* stream) {

  if (!libc_getmntent) {
    libc_getmntent = dlsym(RTLD_NEXT, "getmntent");
  }

  struct mntent* entry = libc_getmntent(stream);
  if (entry != NULL) {
    //~ fprintf(stderr, "%s: mnt_fsname = %s\n", __func__, entry->mnt_fsname);
    //~ fprintf(stderr, "%s: mnt_dir    = %s\n", __func__, entry->mnt_dir);
    //~ fprintf(stderr, "%s: mnt_type   = %s\n", __func__, entry->mnt_type);
    //~ fprintf(stderr, "%s: mnt_opts   = %s\n", __func__, entry->mnt_opts);
    //~ fprintf(stderr, "%s: mnt_freq   = %d\n", __func__, entry->mnt_freq);
    //~ fprintf(stderr, "%s: mnt_passno = %d\n", __func__, entry->mnt_passno);

    // linprocfs bug
    if (strcmp(entry->mnt_fsname, "/sys") == 0) {
      entry->mnt_fsname = "sysfs";
      goto done;
    }

    // none of these paths will be a good library location, ffs
    if (
      strcmp (entry->mnt_dir, "/")          == 0 ||
      strcmp (entry->mnt_dir, "/home")      == 0 ||
      strcmp (entry->mnt_dir, "/tmp")       == 0 ||
      strcmp (entry->mnt_dir, "/usr/home")  == 0 ||
      strcmp (entry->mnt_dir, "/usr/ports") == 0 ||
      strcmp (entry->mnt_dir, "/usr/src")   == 0 ||
      strcmp (entry->mnt_dir, "/var")       == 0 ||
      strncmp(entry->mnt_dir, "/var/", sizeof("/var/") - 1) == 0 ||
      strcmp (entry->mnt_dir, "/zroot")     == 0
    ) {
      entry->mnt_fsname = "nope";
      goto done;
    }

    // same goes for in-memory filesystems
    if (strcmp(entry->mnt_type, "tmpfs") == 0) {
      entry->mnt_fsname = "nope";
      goto done;
    }

    // that is our tmp dir for chroots and stuff
    if (strstr(entry->mnt_dir, ".steam/tmp") != NULL) {
      entry->mnt_fsname = "nope";
      goto done;
    }

    // on the other hand Steam skips entries not starting with /
    if (strcmp(entry->mnt_type, "zfs") == 0) {
      entry->mnt_fsname = "/y/e/s";
    }
  }

done:
  return entry;
}

/* XOpenDisplay spam */

void* (*orig_xcb_connect)(const char*, int*) = NULL;

void* xcb_connect(const char* displayname, int* screen) {

  if (!orig_xcb_connect) {
    orig_xcb_connect = dlsym(RTLD_NEXT, "xcb_connect");
    assert(orig_xcb_connect != NULL);
  }

  static int attempts = 0;
  static __thread void* fail = NULL;

  if (__atomic_fetch_add(&attempts, 1, __ATOMIC_SEQ_CST) > 50) {
    if (fail == NULL) {
      fail = orig_xcb_connect("nope", NULL);
    }
    return fail;
  }

  return orig_xcb_connect(displayname, screen);
}

int (*orig_XCloseDisplay)(void* display) = NULL;

int XCloseDisplay(void* display) {

  if (!orig_XCloseDisplay) {
    orig_XCloseDisplay = dlsym(RTLD_NEXT, "XCloseDisplay");
    assert(orig_XCloseDisplay != NULL);
  }

  if (display != NULL) {
    orig_XCloseDisplay(display);
  }

  return 0;
}
