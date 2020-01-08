#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define FAKE(name) void name() { fprintf(stderr, "fakeudev: %s\n", #name); exit(1); }

FAKE(udev_device_get_devnode);
FAKE(udev_device_get_parent);
FAKE(udev_device_get_parent_with_subsystem_devtype);
FAKE(udev_device_get_property_value);
FAKE(udev_device_get_sysattr_value);
FAKE(udev_device_get_sysname);
FAKE(udev_device_get_syspath);
FAKE(udev_device_new_from_devnum);
FAKE(udev_device_new_from_subsystem_sysname);
FAKE(udev_device_new_from_syspath);
FAKE(udev_enumerate_add_match_subsystem);
FAKE(udev_enumerate_get_list_entry);
FAKE(udev_enumerate_new);
FAKE(udev_enumerate_scan_devices);
FAKE(udev_enumerate_unref);
FAKE(udev_list_entry_get_next);
FAKE(udev_list_entry_get_name);
FAKE(udev_set_log_fn);
FAKE(udev_set_log_priority);

struct udev {};
struct udev_monitor {};
struct udev_device {};

static struct udev         dev;
static struct udev_monitor monitor;
static struct udev_device  device;

struct udev* udev_new(void) {
  return &dev;
}

struct udev* udev_unref(struct udev *udev) {
  return NULL;
}

struct udev_monitor* udev_monitor_new_from_netlink(void* udev, const char* name) {
  return &monitor;
}

void* udev_monitor_new_from_netlink_fd(void* udev, const char* name, int fd) {
  assert(0);
}

int udev_monitor_enable_receiving(struct udev_monitor* udev_monitor) {
  return 0;
}

int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor* udev_monitor, const char* subsystem, const char* devtype) {
  return 0;
}

int udev_monitor_get_fd(struct udev_monitor* udev_monitor) {
  // some paths cause CEF to hang for whatever reason, this one doesn't and it's always conveniently available
  int fd = open("/proc/self/cmdline", O_RDONLY);
  assert(fd != -1);
  return fd;
}

void udev_monitor_unref(struct udev_monitor *udev_monitor) {
  // do nothing
}

struct udev_device* udev_monitor_receive_device(struct udev_monitor* udev_monitor) {
  return &device;
}

const char* udev_device_get_action(struct udev_device* udev_device) {
  return "whatever";
}

const char* udev_device_get_subsystem(struct udev_device* udev_device) {
  return "whatever";
}

struct udev_device* udev_device_unref(struct udev_device* udev_device) {
  return NULL;
}
