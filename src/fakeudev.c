#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define FAKE(name) void name() { fprintf(stderr, "fakeudev: %s\n", #name); exit(1); }

FAKE(udev_device_get_action);
FAKE(udev_device_get_devnum);
FAKE(udev_device_get_devpath);
FAKE(udev_device_get_devtype);
FAKE(udev_device_get_sysnum);
FAKE(udev_device_get_parent);
FAKE(udev_device_get_properties_list_entry);
FAKE(udev_device_get_property_value);
FAKE(udev_device_get_subsystem);
FAKE(udev_device_get_sysname);
FAKE(udev_device_new_from_subsystem_sysname);
FAKE(udev_device_ref);
FAKE(udev_set_log_fn);
FAKE(udev_set_log_priority);

#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/types.h>
#include <linux/hidraw.h>

#ifndef HIDIOCGRAWUNIQ
#define HIDIOCGRAWUNIQ(len) _IOC(_IOC_READ, 'H', 0x08, len)
#endif

struct hidraw_node_info {
  dev_t devnum;
  char* devnode;
  char* syspath;
  char* uevent;
};

static struct hidraw_node_info* hidraw_nodes[100];
static int hidraw_nodes_len = 0;

__attribute__((constructor))
static void init() {

  char path[PATH_MAX];

  for (unsigned int i = 0; i < sizeof(hidraw_nodes) / sizeof(void*); i++) {

    snprintf(path, sizeof(path), "/dev/hidraw%d", i);

    struct stat st;
    if (stat(path, &st) == 0) {

      fprintf(stderr, "fakeudev: found node: %s\n", path);

      int fd = open(path, O_RDONLY);
      if (fd != -1) {

        struct hidraw_devinfo info;
        char name[64];
        char uniq[64];

        memset(&info, 0x0, sizeof(info));
        memset( name, 0x0, sizeof(name));
        memset( uniq, 0x0, sizeof(uniq));

        if (ioctl(fd, HIDIOCGRAWINFO, &info) == -1) {
          perror("fakeudev: ioctl(HIDIOCGRAWINFO)");
        }

        if (ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name) == -1) {
          perror("fakeudev: ioctl(HIDIOCGRAWNAME)");
        }

        if (ioctl(fd, HIDIOCGRAWUNIQ(sizeof(uniq)), uniq) == -1) {
          perror("fakeudev: ioctl(HIDIOCGRAWUNIQ)");
        }

        close(fd);

        fprintf(stderr, "fakeudev: name: %s\n", name);

        char* syspath = NULL;
        char* uevent  = NULL;

        asprintf(&syspath, "/sys/class/hidraw/hidraw%d", i);
        asprintf(&uevent,
          "HID_ID=%04x:%08x:%08x\n"
          "HID_NAME=%s\n"
          "HID_UNIQ=%s\n",
          info.bustype, info.vendor, info.product,
          name,
          uniq
        );

        assert(syspath != NULL);
        assert(uevent  != NULL);

        struct hidraw_node_info* meta = malloc(sizeof(struct hidraw_node_info));

        meta->devnum  = st.st_rdev;
        meta->devnode = strdup(path);
        meta->syspath = syspath;
        meta->uevent  = uevent;

        hidraw_nodes[hidraw_nodes_len] = meta;
        hidraw_nodes_len++;
      }
    }
  }
}

struct udev         {};
struct udev_monitor {};

struct udev_device {
  struct hidraw_node_info* meta;
};

#define ENUMERATOR_HIDRAW 1

struct udev_enumerate {
  uint64_t matches;
  struct udev_list_entry* head;
  struct udev_list_entry* tail;
};

struct udev_list_entry {
  char* name;
  struct udev_list_entry* next;
};

static struct udev           dev;
static struct udev_monitor   monitor;

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

void* udev_monitor_new_from_netlink_fd(void* udev, const char* name, int fd) {
  LOG_ENTRY("%p, \"%s\", %d", udev, name, fd);
  assert(0);
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
  struct udev_enumerate* enumerator = malloc(sizeof(struct udev_enumerate));
  enumerator->matches = 0;
  enumerator->head    = NULL;
  enumerator->tail    = NULL;
  return enumerator;
}

struct udev_list_entry* udev_enumerate_get_list_entry(struct udev_enumerate* enumerator) {
  LOG_ENTRY("%p", enumerator);
  return enumerator->head;
}

int udev_enumerate_add_match_subsystem(struct udev_enumerate* enumerator, const char* subsystem) {
  LOG_ENTRY("%p, \"%s\"", enumerator, subsystem);
  if (strcmp(subsystem, "hidraw") == 0) {
    enumerator->matches |= ENUMERATOR_HIDRAW;
    return 0;
  } else {
    return -1;
  }
}

int udev_enumerate_add_match_property(struct udev_enumerate* udev_enumerate, const char* property, const char* value) {
  LOG_ENTRY("%p, \"%s\", \"%s\"", udev_enumerate, property, value);
  return -1;
}

int udev_enumerate_scan_devices(struct udev_enumerate* enumerator) {
  LOG_ENTRY("%p", enumerator);

  assert(enumerator->head == NULL);
  assert(enumerator->tail == NULL);

  if ((enumerator->matches & ENUMERATOR_HIDRAW) != 0) {
    for (unsigned int i = 0; hidraw_nodes[i] != NULL; i++) {
      struct udev_list_entry* e = malloc(sizeof(struct udev_list_entry));
      e->name = hidraw_nodes[i]->syspath;
      e->next = NULL;
      if (enumerator->tail == NULL) {
        enumerator->head = e;
        enumerator->tail = e;
      } else {
        enumerator->tail->next = e;
        enumerator->tail = e;
      }
    }
  }

  return 0;
}

struct udev_enumerate* udev_enumerate_unref(struct udev_enumerate* enumerator) {
  LOG_ENTRY("%p", enumerator);
  //TODO: free enumerator
  return NULL;
}

const char* udev_list_entry_get_name(struct udev_list_entry* list_entry) {
  LOG_ENTRY("%p [%s]", list_entry, list_entry->name);
  return list_entry->name;
}

struct udev_list_entry* udev_list_entry_get_next(struct udev_list_entry* list_entry) {
  LOG_ENTRY("%p [%p]", list_entry, list_entry->next);
  return list_entry->next;
}

struct udev_device* udev_device_new_from_syspath(struct udev* udev, const char* syspath) {

  LOG_ENTRY("%p, %s", udev, syspath);

  for (unsigned int i = 0; hidraw_nodes[i] != NULL; i++) {
    if (strcmp(hidraw_nodes[i]->syspath, syspath) == 0) {
      struct udev_device* device = malloc(sizeof(struct udev_device));
      device->meta = hidraw_nodes[i];
      return device;
    }
  }

  return NULL;
}

const char* udev_device_get_syspath(struct udev_device* device) {
  LOG_ENTRY("%p", device);
  return device->meta->syspath;
}

const char* udev_device_get_devnode(struct udev_device* device) {
  LOG_ENTRY("%p", device);
  return device->meta->devnode;
}

struct udev_device* udev_device_get_parent_with_subsystem_devtype(struct udev_device* device, const char* subsystem, const char* devtype) {
  LOG_ENTRY("%p, %s, %s", device, subsystem, devtype);

  if (strcmp(subsystem, "hid") == 0) {
    return device; // Parent? What parent?
  }

  if (strcmp(subsystem, "usb") == 0 && (strcmp(devtype, "usb_device") == 0 || strcmp(devtype, "usb_interface") == 0)) {
    return device;
  }

  return NULL;
}

const char* udev_device_get_sysattr_value(struct udev_device* device, const char* sysattr) {
  LOG_ENTRY("%p, %s", device, sysattr);

  if (strcmp(sysattr, "uevent") == 0) {
    return device->meta->uevent;
  }

  if (strcmp(sysattr, "manufacturer") == 0) {
    return ""; // ?
  }

  if (strcmp(sysattr, "product") == 0) {
    return ""; // ?
  }

  if (strcmp(sysattr, "bcdDevice") == 0) {
    return ""; // ?
  }

  if (strcmp(sysattr, "bInterfaceNumber") == 0) {
    return "0"; // ?
  }

  return NULL;
}

struct udev_device* udev_device_new_from_devnum(struct udev* udev, char type, dev_t devnum) {
  LOG_ENTRY("%p, %c, %ld", udev, type, devnum);

  for (unsigned int i = 0; hidraw_nodes[i] != NULL; i++) {
    if (hidraw_nodes[i]->devnum == devnum) {
      return udev_device_new_from_syspath(udev, hidraw_nodes[i]->syspath);
    }
  }

  return NULL;
}

struct udev_device* udev_device_unref(struct udev_device* device) {
  LOG_ENTRY("%p", device);
  //TODO: free device
  return NULL;
}
