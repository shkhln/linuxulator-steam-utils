#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// show CEF version

static int (*orig_cef_initialize)(void*, void*, void*, void*) = NULL;

int cef_initialize(void* args, void* settings, void* application, void* windows_sandbox_info) {

  if (!orig_cef_initialize) {
    orig_cef_initialize = dlsym(RTLD_NEXT, "cef_initialize");
    assert(orig_cef_initialize != NULL);
  }

  int (*cef_version_info)(int) = dlsym(RTLD_NEXT, "cef_version_info");
  assert(cef_version_info != NULL);

  int cef_major    = cef_version_info(0);
  int cef_minor    = cef_version_info(1);
  int cef_patch    = cef_version_info(2);
  int cef_commit   = cef_version_info(3);
  int chrome_major = cef_version_info(4);
  int chrome_minor = cef_version_info(5);
  int chrome_build = cef_version_info(6);
  int chrome_patch = cef_version_info(7);

  fprintf(stderr, "[[CEF version = %d.%d.%d.%d, Chrome version = %d.%d.%d.%d]]\n",
    cef_major, cef_minor, cef_patch, cef_commit, chrome_major, chrome_minor, chrome_build,chrome_patch);

  return orig_cef_initialize(args, settings, application, windows_sandbox_info);
}

#if __FreeBSD_version < 1300139

/* SO_PASSCRED workaround */

#include <execinfo.h>
#include <sys/types.h>
#include <sys/socket.h>

static int (*libc_setsockopt)(int, int, int, const void*, socklen_t) = NULL;

int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {

  if (!libc_setsockopt) {
    libc_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
  }

  if (optname == SO_PASSCRED) {
    return 0;
  }

  return libc_setsockopt(s, level, optname, optval, optlen);
}

static struct msghdr* copy_and_add_credentials(struct msghdr* msg) {

  struct msghdr* result_msg = malloc(sizeof(struct msghdr));
  memcpy(result_msg, msg, sizeof(struct msghdr));

  int   result_cmsg_len = msg->msg_controllen + CMSG_SPACE(sizeof(struct ucred));
  char* result_cmsg     = malloc(result_cmsg_len);

  memset(result_cmsg, 0, result_cmsg_len);
  memcpy(result_cmsg, msg->msg_control, msg->msg_controllen);

  result_msg->msg_control    = result_cmsg;
  result_msg->msg_controllen = result_cmsg_len;

  struct cmsghdr* cmsg_in  = NULL;
  struct cmsghdr* cmsg_out = NULL;
  while (true) {
    if (!cmsg_in) {
      cmsg_in  = CMSG_FIRSTHDR(msg);
      cmsg_out = CMSG_FIRSTHDR(result_msg);
    } else {
      cmsg_in  = CMSG_NXTHDR(msg, cmsg_in);
      cmsg_out = CMSG_NXTHDR(result_msg, cmsg_out);
    }

    if (cmsg_in != NULL) {
      memcpy(cmsg_out, cmsg_in, cmsg_in->cmsg_len);
    } else {

      cmsg_out->cmsg_len   = CMSG_LEN(sizeof(struct ucred));
      cmsg_out->cmsg_level = SOL_SOCKET;
      cmsg_out->cmsg_type  = SCM_CREDENTIALS;

      struct ucred* ucred = (struct ucred*)CMSG_DATA(cmsg_out);

      ucred->pid = getpid();
      ucred->uid = getuid();
      ucred->gid = getgid();

      break;
    }
  }

  return result_msg;
}

static ssize_t (*libc_sendmsg)(int s, const struct msghdr* msg, int flags) = NULL;

ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {

  if (!libc_sendmsg) {
    libc_sendmsg = dlsym(RTLD_NEXT, "sendmsg");
  }

  ssize_t nbytes = -1;

  void* buffer[2];
  int nframes = backtrace(buffer, 2);
  assert(nframes == 2);

  char* caller_str = *backtrace_symbols(buffer + 1, 1);
  assert(caller_str != NULL);

  char* p = strrchr(caller_str, '/');

  if (p && strncmp("libcef.so", p + 1, sizeof("libcef.so") - 1) == 0) {
    struct msghdr* xmsg = copy_and_add_credentials((struct msghdr*)msg);
    nbytes = libc_sendmsg(s, xmsg, flags);
    free(xmsg->msg_control);
    free(xmsg);
  } else {
    nbytes = libc_sendmsg(s, msg, flags);
  }

  return nbytes;
}

#endif // __FreeBSD_version < 1300139
