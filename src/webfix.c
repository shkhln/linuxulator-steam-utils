#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

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
