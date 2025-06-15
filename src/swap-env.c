#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void swap_env_vars(const char* target_var, const char* save_var, const char* load_var) {

  char* old_value = getenv(target_var);
  if (old_value != NULL) {
    setenv(save_var, old_value, 1);
  } else {
    unsetenv(save_var);
  }

  char* new_value = getenv(load_var);
  if (new_value != NULL) {
    setenv(target_var, new_value, 1);
  } else {
    unsetenv(target_var);
  }
}

int main(int argc, char* argv[]) {

  if (argc < 2) {
    errx(EXIT_FAILURE, "no arguments passed");
  }

#ifdef FBSD_TO_LINUX
  swap_env_vars("LD_LIBRARY_PATH",    "LSU_FBSD_LD_LIBRARY_PATH",     "LSU_LINUX_LD_LIBRARY_PATH");
  swap_env_vars("LD_PRELOAD",         "LSU_FBSD_LD_PRELOAD",          "LSU_LINUX_LD_PRELOAD");
  swap_env_vars("LIBGL_DRIVERS_PATH", "LSU_FBSD_LIBGL_DRIVERS_PATH",  "LSU_LINUX_LIBGL_DRIVERS_PATH");
  swap_env_vars("PATH",               "LSU_FBSD_PATH",                "LSU_LINUX_PATH");
#else
  swap_env_vars("LD_LIBRARY_PATH",    "LSU_LINUX_LD_LIBRARY_PATH",    "LSU_FBSD_LD_LIBRARY_PATH");
  swap_env_vars("LD_PRELOAD",         "LSU_LINUX_LD_PRELOAD",         "LSU_FBSD_LD_PRELOAD");
  swap_env_vars("LIBGL_DRIVERS_PATH", "LSU_LINUX_LIBGL_DRIVERS_PATH", "LSU_FBSD_LIBGL_DRIVERS_PATH");
  swap_env_vars("PATH",               "LSU_LINUX_PATH",               "LSU_FBSD_PATH");
#endif

  execvp(argv[1], &argv[1]);
  err(EXIT_FAILURE, "execvp(%s)", argv[1]);
}
