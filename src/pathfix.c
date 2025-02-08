#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Let's rewrite some paths... */

static const char* redirect(const char* path) {

  // Point Steam to a proper root certificate bundle
  if (strcmp(path, "/etc/ssl/certs/ca-certificates.crt") == 0) {
    return "/etc/ssl/cert.pem";
  }

  // Let's pretend this file exists
  if (strcmp(path, "/usr/sbin/lsof") == 0) {
    return "/bin/sh";
  }

  // Lie to Steam about /home being a directory.
  // The Steam client skips symlinks in Add Library Folder / Add a (non-Steam) Gamed dialogs and,
  // if it was started from /home/... path, it will get quite confused without this workaround.
  if (strcmp(path, "//home") == 0) {
    return "/usr/home";
  }

  // Also trick Linuxulator into listing actual /usr contents instead of /compat/linux/usr
  if (strcmp(path, "/usr") == 0) {
    return "/usr/local/..";
  }

  // Steam calls this from ubuntu12_32/steam(CanSetClientBeta+0x2d) 44 times.
  // Since this is apparently a very important check, it's only natural that we ignore it.
  if (strcmp(path, "./.writable") == 0) {
    return "/dev/null";
  }

  return path;
}

static int (*libc_access)(const char*, int) = NULL;

int access(const char* path, int mode) {

  if (!libc_access) {
    libc_access = dlsym(RTLD_NEXT, "access");
  }

  const char* p = redirect(path);
  if (p != NULL) {
    return libc_access(p, mode);
  } else {
    errno = EACCES;
    return -1;
  }
}

static FILE* (*libc_fopen64)(const char*, const char*) = NULL;

FILE* fopen64(const char* path, const char* mode) {

  if (!libc_fopen64) {
    libc_fopen64 = dlsym(RTLD_NEXT, "fopen64");
  }

  const char* p = redirect(path);
  if (p != NULL) {
    FILE* f = libc_fopen64(p, mode);
    //~ fprintf(stderr, "%s(%s, %s) -> %p\n", __func__, path, mode, f);
    return f;
  } else {
    errno = EACCES;
    return NULL;
  }
}

static FILE* (*libc_fopen)(const char*, const char*) = NULL;

FILE* fopen(const char* path, const char* mode) {

  if (!libc_fopen) {
    libc_fopen = dlsym(RTLD_NEXT, "fopen");
  }

  const char* p = redirect(path);
  if (p != NULL) {
    FILE* f = libc_fopen(p, mode);
    //~ fprintf(stderr, "%s(%s, %s) -> %p\n", __func__, path, mode, f);
    return f;
  } else {
    errno = EACCES;
    return NULL;
  }
}

#include <sys/stat.h>

static int (*libc_xstat64) (int, const char*, struct stat64*) = NULL;

int __xstat64(int ver, const char* path, struct stat64* stat_buf) {

  if (!libc_xstat64) {
    libc_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
  }

  const char* p = redirect(path);
  if (p != NULL) {
    return libc_xstat64(ver, p, stat_buf);
  } else {
    errno = EACCES;
    return -1;
  }
}

static int (*libc_lxstat64)(int, const char*, struct stat64*) = NULL;

int __lxstat64(int ver, const char* path, struct stat64* stat_buf) {

  if (!libc_lxstat64) {
    libc_lxstat64 = dlsym(RTLD_NEXT, "__lxstat64");
  }

  const char* p = redirect(path);
  if (p != NULL) {
    return libc_lxstat64(ver, p, stat_buf);
  } else {
    errno = EACCES;
    return -1;
  }
}

#include <dirent.h>

static int (*libc_scandir64)(const char*, struct dirent64***,
  int (*)(const struct dirent64*), int (*)(const struct dirent64**, const struct dirent64**)) = NULL;

int scandir64(const char* dir, struct dirent64*** namelist,
  int (*sel)(const struct dirent64*), int (*cmp)(const struct dirent64**, const struct dirent64**))
{
  if (!libc_scandir64) {
    libc_scandir64 = dlsym(RTLD_NEXT, "scandir64");
  }

  const char* p = redirect(dir);
  if (p != NULL) {
    return libc_scandir64(p, namelist, sel, cmp);
  } else {
    errno = EACCES;
    return -1;
  }
}

/*
  Some games (Torchlight 2, Infinity Engine "Enhanced Edition" ports) invoke mkdir with a sequence of absolute paths
  expecting either success or an EEXIST error, while Linuxulator returns EACCES for /home and /usr/home:

    linux_mkdir("/home")            --> /compat/linux             exists        --> mkdir("/compat/linux/home") --> EACCES
    linux_mkdir("/home/<user>")     --> /compat/linux/home        doesn't exist --> mkdir("/home/<user>")       --> EEXIST
    linux_mkdir("/home/<user>/foo") --> /compat/linux/home/<user> doesn't exist --> mkdir("/home/<user>/foo")   --> OK | EEXIST
*/

#include <errno.h>
#include <sys/stat.h>

static int (*libc_mkdir)(const char*, mode_t) = NULL;

int mkdir(const char* path, mode_t mode) {

  if (!libc_mkdir) {
    libc_mkdir = dlsym(RTLD_NEXT, "mkdir");
  }

  int err = libc_mkdir(path, mode);
  if (err == -1 && errno == EACCES && (strcmp(path, "/home") == 0 || strcmp(path, "/usr/home") == 0)) {
    errno = EEXIST;
  }

  return err;
}
