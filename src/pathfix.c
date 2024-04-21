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

/* hidapi */

#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <linux/hidraw.h>

int (*libc_open)(const char* path, int flags,  ...) = NULL;
int (*libc_open64)(const char* path, int flags,  ...) = NULL;

#ifndef HIDIOCGRAWUNIQ
#define HIDIOCGRAWUNIQ(len) _IOC(_IOC_READ, 'H', 0x08, len)
#endif

static int open_impl(const char* path, int flags, va_list args) {

  if (!libc_open) {
    libc_open = dlsym(RTLD_NEXT, "open");
  }

  mode_t mode = 0;
  if (flags & O_CREAT) {
    mode = va_arg(args, mode_t);
  }

  int fd = libc_open(path, flags, mode);
  fprintf(stderr, "%s(%s, %d, %d) -> %d [%d]\n", __func__, path, flags, mode, fd, strncmp(path, "/sys/class/hidraw", sizeof("/sys/class/hidraw") - 1) == 0);

  if (fd == -1 && strncmp(path, "/sys/class/hidraw/hidraw", sizeof("/sys/class/hidraw/hidraw") - 1) == 0) {

    errno = 0;
    long idx = strtol(&path[sizeof("/sys/class/hidraw/hidraw") - 1], NULL, 10);
    assert(errno != ERANGE && errno != EINVAL);

    char syspath[PATH_MAX];
    snprintf(syspath, sizeof(syspath), "/sys/class/hidraw/hidraw%ld/device/uevent", idx);
    if (strcmp(path, syspath) == 0) {

      fprintf(stderr, "%s: faking uevent\n", __func__);

      char dev_path[PATH_MAX];
      snprintf(dev_path, sizeof(dev_path), "/dev/hidraw%ld", idx);

      int dev_fd = libc_open(dev_path, O_RDONLY);
      if (dev_fd == -1) {
        return -1;
      }

      fd = syscall(SYS_memfd_create, "uevent shim", 0);
      assert(fd != -1);

      struct hidraw_devinfo info;
      memset(&info, 0x0, sizeof(info));

      if (ioctl(dev_fd, HIDIOCGRAWINFO, &info) != -1) {
        fprintf(stderr, "HID_ID=%04x:%08x:%08x\n", info.bustype, info.vendor, info.product);
        dprintf(fd, "HID_ID=%04x:%08x:%08x\n", info.bustype, info.vendor, info.product);
      }

      char buf[256];
      memset(buf, 0x0, sizeof(buf));

      if (ioctl(dev_fd, HIDIOCGRAWNAME(256), buf) != -1) {
        fprintf(stderr, "HID_NAME=%s\n", buf);
        dprintf(fd, "HID_NAME=%s\n", buf);
      }

      memset(buf, 0x0, sizeof(buf));

      if (ioctl(dev_fd, HIDIOCGRAWUNIQ(256), buf) != -1) {
        fprintf(stderr, "HID_UNIQ=%s\n", buf);
        dprintf(fd, "HID_UNIQ=%s\n", buf);
      }

      close(dev_fd);

      lseek(fd, SEEK_SET, 0);

      return fd;
    }

    snprintf(syspath, sizeof(syspath), "/sys/class/hidraw/hidraw%ld/device/report_descriptor", idx);
    if (strcmp(path, syspath) == 0) {

      fprintf(stderr, "%s: faking report_descriptor\n", __func__);

      char dev_path[PATH_MAX];
      snprintf(dev_path, sizeof(dev_path), "/dev/hidraw%ld", idx);

      int dev_fd = libc_open(dev_path, O_RDONLY);
      if (dev_fd == -1) {
        return -1;
      }

      struct hidraw_report_descriptor desc;
      memset(&desc, 0x0, sizeof(desc));

      if (ioctl(dev_fd, HIDIOCGRDESCSIZE, &desc.size) == -1) {
        perror("HIDIOCGRDESCSIZE");
        goto fail;
      }

      if (ioctl(dev_fd, HIDIOCGRDESC, &desc) == -1) {
        perror("HIDIOCGRDESC");
        goto fail;
      }

      close(dev_fd);

      fd = syscall(SYS_memfd_create, "report_descriptor shim", 0);
      assert(fd != -1);

      int nbytes = write(fd, desc.value, desc.size);
      assert(nbytes == (int)desc.size);

      lseek(fd, SEEK_SET, 0);

      return fd;

fail:
      close(dev_fd);
      return -1;
    }
  }

  return fd;
}

static int open64_impl(const char* path, int flags, va_list args) {

  if (!libc_open64) {
    libc_open64 = dlsym(RTLD_NEXT, "open64");
  }

  mode_t mode = 0;
  if (flags & O_CREAT) {
    mode = va_arg(args, mode_t);
  }

  fprintf(stderr, "%s(%s, %d, %d)\n", __func__, path, flags, mode);

  return libc_open64(path, flags, mode);
}

int open(const char* path, int flags,  ...) {
  va_list _args_;
  va_start(_args_, flags);
  int fd = open_impl(path, flags, _args_);
  va_end(_args_);
  return fd;
}

int open64(const char* path, int flags,  ...) {
  va_list _args_;
  va_start(_args_, flags);
  int fd = open64_impl(path, flags, _args_);
  va_end(_args_);
  return fd;
}
