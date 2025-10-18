#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 0

#include <sys/syscall.h>

extern char* program_invocation_short_name;

#define __HEAD(head, ...) head
#define __TAIL(head, ...) __VA_ARGS__

#define LOG(...) fprintf(stderr, "fakeudev: [%.50s][%d:%ld] " __HEAD(__VA_ARGS__) "\n", program_invocation_short_name, getpid(), syscall(SYS_gettid), __TAIL(__VA_ARGS__))

#define LOG_ENTRY(fmt, ...) __builtin_choose_expr(__builtin_strcmp("" fmt, "") == 0, LOG("%s()",       __func__), LOG("%s("    fmt ")", __func__, ## __VA_ARGS__))
#define LOG_EXIT( fmt, ...) __builtin_choose_expr(__builtin_strcmp("" fmt, "") == 0, LOG("%s -> void", __func__), LOG("%s -> " fmt,     __func__, ## __VA_ARGS__))

#else

#define LOG(...)
#define LOG_ENTRY(fmt, ...)
#define LOG_EXIT( fmt, ...)

#endif

#define FAKE(name) void name() { fprintf(stderr, "fakeudev: %s is missing\n", #name); exit(1); }

FAKE(udev_device_get_action);
FAKE(udev_device_get_current_tags_list_entry);
FAKE(udev_device_get_devlinks_list_entry);
FAKE(udev_device_get_devnode);
FAKE(udev_device_get_devnum);
FAKE(udev_device_get_devpath);
FAKE(udev_device_get_devtype);
FAKE(udev_device_get_driver);
FAKE(udev_device_get_is_initialized);
FAKE(udev_device_get_parent_with_subsystem_devtype);
FAKE(udev_device_get_parent);
FAKE(udev_device_get_properties_list_entry);
FAKE(udev_device_get_property_value);
FAKE(udev_device_get_seqnum);
FAKE(udev_device_get_subsystem);
FAKE(udev_device_get_sysattr_list_entry);
FAKE(udev_device_get_sysattr_value);
FAKE(udev_device_get_sysname);
FAKE(udev_device_get_sysnum);
FAKE(udev_device_get_syspath);
FAKE(udev_device_get_tags_list_entry);
FAKE(udev_device_get_udev);
FAKE(udev_device_get_usec_since_initialized);
FAKE(udev_device_has_current_tag);
FAKE(udev_device_has_tag);
FAKE(udev_device_new_from_device_id);
FAKE(udev_device_new_from_environment);
FAKE(udev_device_new_from_subsystem_sysname);
FAKE(udev_device_new_from_syspath);
FAKE(udev_device_ref);
FAKE(udev_device_set_sysattr_value);
FAKE(udev_device_unref);
FAKE(udev_enumerate_add_match_is_initialized);
FAKE(udev_enumerate_add_match_parent);
FAKE(udev_enumerate_add_match_sysattr);
FAKE(udev_enumerate_add_match_sysname);
FAKE(udev_enumerate_add_match_tag);
FAKE(udev_enumerate_add_nomatch_subsystem);
FAKE(udev_enumerate_add_nomatch_sysattr);
FAKE(udev_enumerate_add_syspath);
FAKE(udev_enumerate_get_udev);
FAKE(udev_enumerate_ref);
FAKE(udev_enumerate_scan_subsystems);
FAKE(udev_get_log_priority);
FAKE(udev_get_userdata);
FAKE(udev_hwdb_get_properties_list_entry);
FAKE(udev_hwdb_new);
FAKE(udev_hwdb_ref);
FAKE(udev_hwdb_unref);
FAKE(udev_list_entry_get_by_name);
FAKE(udev_list_entry_get_name);
FAKE(udev_list_entry_get_next);
FAKE(udev_list_entry_get_value);
FAKE(udev_monitor_filter_add_match_tag);
FAKE(udev_monitor_filter_remove);
FAKE(udev_monitor_filter_update);
FAKE(udev_monitor_get_udev);
FAKE(udev_monitor_ref);
FAKE(udev_monitor_set_receive_buffer_size);
FAKE(udev_queue_flush);
FAKE(udev_queue_get_fd);
FAKE(udev_queue_get_kernel_seqnum);
FAKE(udev_queue_get_queue_is_empty);
FAKE(udev_queue_get_queued_list_entry);
FAKE(udev_queue_get_seqnum_is_finished);
FAKE(udev_queue_get_seqnum_sequence_is_finished);
FAKE(udev_queue_get_udev_is_active);
FAKE(udev_queue_get_udev_seqnum);
FAKE(udev_queue_get_udev);
FAKE(udev_queue_new);
FAKE(udev_queue_ref);
FAKE(udev_queue_unref);
FAKE(udev_ref);
FAKE(udev_set_log_fn);
FAKE(udev_set_log_priority);
FAKE(udev_set_userdata);
FAKE(udev_util_encode_string);

struct udev           {};
struct udev_monitor   {};
struct udev_device    {};
struct udev_enumerate {};

static struct udev           dev;
static struct udev_monitor   monitor;
static struct udev_enumerate enumerator;

struct udev* udev_new() {
  LOG_ENTRY();
  return &dev;
}

struct udev* udev_unref(struct udev* udev) {
  LOG_ENTRY("%p", udev);
  return NULL;
}

struct udev_monitor* udev_monitor_new_from_netlink(void* udev, const char* name) {
  LOG_ENTRY("%p, \"%s\"", udev, name);
  return &monitor;
}

int udev_monitor_enable_receiving(struct udev_monitor* udev_monitor) {
  LOG_ENTRY("%p", udev_monitor);
  return 0;
}

int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor* udev_monitor, const char* subsystem, const char* devtype) {
  LOG_ENTRY("%p, \"%s\", \"%s\"", udev_monitor, subsystem, devtype);
  return 0;
}

static int monitor_fd = -1;

__attribute__((constructor))
static void init_monitor_fd() {

  int inout[2];
  {
    int err = pipe(inout);
    assert(err == 0);
  }

  monitor_fd = inout[1];
}

int udev_monitor_get_fd(struct udev_monitor* udev_monitor) {
  LOG_ENTRY("%p", udev_monitor);
  return monitor_fd;
}

void udev_monitor_unref(struct udev_monitor *udev_monitor) {
  LOG_ENTRY("%p", udev_monitor);
  // do nothing
}

struct udev_device* udev_monitor_receive_device(struct udev_monitor* udev_monitor) {
  LOG_ENTRY("%p", udev_monitor);
  return NULL;
}

struct udev_enumerate* udev_enumerate_new(struct udev* udev) {
  LOG_ENTRY("%p", udev);
  return &enumerator;
}

struct udev_list_entry* udev_enumerate_get_list_entry(struct udev_enumerate* udev_enumerate) {
  LOG_ENTRY("%p", udev_enumerate);
  return NULL;
}

// Chrome (and derived projects) expect only 0 as the return value:
// https://source.chromium.org/chromium/chromium/src/+/main:device/udev_linux/udev_watcher.cc;drc=ec84fc326df161fec278698d6b9cfa48aa67d3eb;l=93
int udev_enumerate_add_match_subsystem(struct udev_enumerate* udev_enumerate, const char* subsystem) {
  LOG_ENTRY("%p, \"%s\"", udev_enumerate, subsystem);
  return 0;
}

int udev_enumerate_add_match_property(struct udev_enumerate* udev_enumerate, const char* property, const char* value) {
  LOG_ENTRY("%p, \"%s\", \"%s\"", udev_enumerate, property, value);
  return -1;
}

int udev_enumerate_scan_devices(struct udev_enumerate* udev_enumerate) {
  LOG_ENTRY("%p", udev_enumerate);
  return -1;
}

struct udev_enumerate* udev_enumerate_unref(struct udev_enumerate* udev_enumerate) {
  LOG_ENTRY("%p", udev_enumerate);
  return NULL;
}

struct udev_device* udev_device_new_from_devnum(struct udev* udev, char type, dev_t devnum) {
  LOG_ENTRY("%p, %c, %ld", udev, type, (unsigned long int)devnum);
  return NULL;
}
