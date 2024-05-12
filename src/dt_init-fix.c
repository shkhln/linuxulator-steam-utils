#include <assert.h>
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int    _argc = 0;
static char** _argv = NULL;
static char** _env  = NULL;

static void* (*orig_dlopen)(const char*, int) = NULL;
static char* steamclient_path = NULL;
static char* steamclient_alt  = NULL;

__attribute__((constructor))
static void init(int argc, char** argv, char** env) {

  _argc = argc;
  _argv = argv;
  _env  = env;

  orig_dlopen = dlvsym(RTLD_DEFAULT, "dlopen", "FBSD_1.0");
  assert(orig_dlopen != NULL);

  steamclient_path = getenv("LSU_STEAMCLIENT_PATH");
  steamclient_alt  = getenv("LSU_STEAMCLIENT_ALT_PATH");
  assert(steamclient_path != NULL);
  assert(steamclient_alt  != NULL);
}

static void call_constructors(void* obj, int argc, char** argv, char** env) {

  //~ fprintf(stderr, "%s: %d, %p, %p\n", __func__, argc, argv, env);

  Link_map* map = NULL;

  int err = dlinfo(obj, RTLD_DI_LINKMAP, &map);
  assert(err == 0);

  while (map != NULL) {
    void*  dt_init            = NULL;
    void** dt_init_array      = NULL;
    int    dt_init_array_size = 0;

    for (const Elf_Dyn* dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
      switch (dyn->d_tag) {
        case 0x42420000 + DT_INIT:
          dt_init = map->l_addr + dyn->d_un.d_ptr;
          break;
        case 0x42420000 + DT_INIT_ARRAY:
          dt_init_array = (void**)(map->l_addr + dyn->d_un.d_ptr);
          break;
        case DT_INIT_ARRAYSZ:
          dt_init_array_size = dyn->d_un.d_ptr / sizeof(void*);
          break;
      }
    }

    if (dt_init != NULL) {
      void (*init)(int, char**, char**) = dt_init;
      fprintf(stderr, "%s: calling init function for %s at %p\n", __FILE__, map->l_name, init);
      init(argc, argv, env);
    }

    if (dt_init_array != NULL) {
      for (int i = 0; i < dt_init_array_size; i++) {
        if ((uintptr_t)dt_init_array[i] != 0 && (uintptr_t)dt_init_array[i] != 1) {
          void (*init)(int, char**, char**) = dt_init_array[i];
          fprintf(stderr, "%s: calling init function %d for %s at %p\n", __FILE__, i, map->l_name, init);
          init(argc, argv, env);
        }
      }
    }

    map = map->l_next;
  }
}

void* dlopen(const char* path, int mode) {

  //~ fprintf(stderr, "%s: %s\n", __func__, path);

  if (path != NULL && strcmp(path, steamclient_path) == 0 && (mode & RTLD_NOLOAD) == 0) {
    void* obj = orig_dlopen(steamclient_alt, mode);
    call_constructors(obj, _argc, _argv, _env);
    return obj;
  } else {
    return orig_dlopen(path, mode);
  }
}
