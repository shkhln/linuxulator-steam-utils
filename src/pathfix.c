#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
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

  return path;
}

static int (*libc_access)(const char*, int) = NULL;

int access(const char* path, int mode) {

  if (!libc_access) {
    libc_access = dlsym(RTLD_NEXT, "access");
  }

  return libc_access(redirect(path), mode);
}

static FILE* (*libc_fopen64)(const char*, const char*) = NULL;

FILE* fopen64(const char* path, const char* mode) {

  if (!libc_fopen64) {
    libc_fopen64 = dlsym(RTLD_NEXT, "fopen64");
  }

  return libc_fopen64(redirect(path), mode);
}

#include <sys/stat.h>

static int (*libc_lxstat64)(int, const char*, struct stat64*) = NULL;

int __lxstat64(int ver, const char* path, struct stat64* stat_buf) {

  if (!libc_lxstat64) {
    libc_lxstat64 = dlsym(RTLD_NEXT, "__lxstat64");
  }

  return libc_lxstat64(ver, redirect(path), stat_buf);
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

  return libc_scandir64(redirect(dir), namelist, sel, cmp);
}
