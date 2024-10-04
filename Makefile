.if !defined(OSVERSION)
OSVERSION != awk '/^\#define[[:blank:]]__FreeBSD_version/ {print $$3}' < /usr/include/sys/param.h # from bsd.port.mk
.endif

PROJECT = steam-utils

BUILD_DIR ?= .
PREFIX    ?= /opt

CFLAGS = --sysroot=/compat/linux -std=c99 -Wall -Wextra -Wno-unused-parameter -D__FreeBSD_version=${OSVERSION}

LIBS  = lib32/steamfix/steamfix.so       \
        lib32/fakenm/libnm.so.0          \
        lib32/fakenm/libnm-glib.so.4     \
        lib32/fakepulse/libpulse.so.0    \
        lib64/fakepulse/libpulse.so.0    \
        lib32/fakeudev/libudev.so.0      \
        lib64/fakeudev/libudev.so.0      \
        lib32/fakeudev/libudev.so.1      \
        lib64/fakeudev/libudev.so.1      \
        lib32/noepollexcl/noepollexcl.so \
        lib64/noepollexcl/noepollexcl.so \
        lib32/pathfix/pathfix.so         \
        lib64/pathfix/pathfix.so         \
        lib32/protonfix/protonfix.so     \
        lib64/protonfix/protonfix.so     \
        lib32/shmfix/shmfix.so           \
        lib64/shmfix/shmfix.so           \
        lib64/webfix/webfix.so           \
        lib32/steamclient/dt_init-fix.so

LIBS := ${LIBS:C|(.*)|$(BUILD_DIR)/\1|}

build: $(LIBS)

.for b in 32 64

$(BUILD_DIR)/lib$(b)/steamfix/steamfix.so: src/steamfix.c src/pathfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)/steamfix
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/steamfix.c src/pathfix.c -pthread -ldl -lm

$(BUILD_DIR)/lib$(b)/fakenm/libnm.so.0: src/fakenm.c
	mkdir -p $(BUILD_DIR)/lib$(b)/fakenm
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/fakenm.c

$(BUILD_DIR)/lib$(b)/fakenm/libnm-glib.so.4: $(BUILD_DIR)/lib$(b)/fakenm/libnm.so.0
	ln -s libnm.so.0 $(.TARGET)

$(BUILD_DIR)/lib$(b)/fakepulse/libpulse.so.0: src/fakepulse.c
	mkdir -p $(BUILD_DIR)/lib$(b)/fakepulse
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/fakepulse.c

$(BUILD_DIR)/lib$(b)/fakeudev/libudev.so.0: src/fakeudev.c
	mkdir -p $(BUILD_DIR)/lib$(b)/fakeudev
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/fakeudev.c

$(BUILD_DIR)/lib$(b)/fakeudev/libudev.so.1: $(BUILD_DIR)/lib$(b)/fakeudev/libudev.so.0
	ln -s libudev.so.0 $(.TARGET)

$(BUILD_DIR)/lib$(b)/pathfix/pathfix.so: src/pathfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)/pathfix
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/pathfix.c -ldl

$(BUILD_DIR)/lib$(b)/protonfix/protonfix.so: src/protonfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)/protonfix
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/protonfix.c -pthread -ldl

$(BUILD_DIR)/lib$(b)/webfix/webfix.so: src/webfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)/webfix
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/webfix.c -pthread -ldl -lm

$(BUILD_DIR)/lib$(b)/noepollexcl/noepollexcl.so: src/noepollexcl.c
	mkdir -p $(BUILD_DIR)/lib$(b)/noepollexcl
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/noepollexcl.c -ldl

$(BUILD_DIR)/lib$(b)/shmfix/shmfix.so: src/shmfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)/shmfix
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/shmfix.c -ldl -lrt
.endfor

$(BUILD_DIR)/lib32/steamclient/dt_init-fix.so: src/dt_init-fix.c
	mkdir -p $(BUILD_DIR)/lib32/steamclient
	cc -m32 -mstack-alignment=16 -mstackrealign -std=c99 -Wall -Wextra -fPIC -shared -o $(.TARGET) src/dt_init-fix.c

clean:
.for f in $(LIBS)
.  if exists($f)
	rm $f
.  endif
.endfor

install:
	install -d $(PREFIX)/$(PROJECT)
	install -d $(PREFIX)/$(PROJECT)/bin
	install -d $(PREFIX)/$(PROJECT)/lib32
	install -d $(PREFIX)/$(PROJECT)/lib64
	install -d $(PREFIX)/$(PROJECT)/lxbin
.for f in $(LIBS)
	install -d `dirname $(PREFIX)/$(PROJECT)/${f:C|$(BUILD_DIR)/(.*)|\1|}`
	install $(f) $(PREFIX)/$(PROJECT)/${f:C|$(BUILD_DIR)/(.*)|\1|}
.endfor
	install bin/.dpkgs.rb bin/.utils.rb bin/lsu-* bin/steam $(PREFIX)/$(PROJECT)/bin
	install \
 lxbin/curl-config \
 lxbin/dbus-launch \
 lxbin/file* \
 lxbin/lsof \
 lxbin/lspci \
 lxbin/lsu-* \
 lxbin/tar \
 lxbin/xdg-user-dir \
 lxbin/xrandr \
 lxbin/zenity \
 $(PREFIX)/$(PROJECT)/lxbin
.for t in LSU_FreeBSD_Wine LSU_Proton_8_chroot LSU_Scout_chroot LSU_Sniper_chroot
	install -d $(PREFIX)/$(PROJECT)/tools/$(t)
	install -m 0644 tools/$(t)/compatibilitytool.vdf $(PREFIX)/$(PROJECT)/tools/$(t)
	install -m 0644 tools/$(t)/toolmanifest.vdf      $(PREFIX)/$(PROJECT)/tools/$(t)
	install tools/$(t)/run.sh tools/$(t)/*.rb $(PREFIX)/$(PROJECT)/tools/$(t)
.endfor

deinstall:
.if exists($(PREFIX)/$(PROJECT))
	rm -r -I $(PREFIX)/$(PROJECT)
.endif

NVIDIA_DEPS != \
	case `sysctl -q hw.nvidia.version | sed -n "s/hw.nvidia.version: NVIDIA UNIX x86_64 Kernel Module  \([0-9]*\).*/\1/p"` in \
	  '')  echo '' ;;                   \
	  340) echo linux-nvidia-libs-340;; \
	  390) echo linux-nvidia-libs-390;; \
	  470) echo linux-nvidia-libs-470;; \
	  *)   echo linux-nvidia-libs;;     \
	esac

DEPS = ruby                      \
       ca_root_nss               \
       gtar                      \
       liberation-fonts-ttf      \
       linux_libusb-c7           \
       linux-c7-alsa-plugins-oss \
       linux-c7-dbus-libs        \
       linux-c7-devtools         \
       linux-c7-dri              \
       linux-c7-gtk2             \
       linux-c7-nss              \
       zenity

dependencies:
	pkg install -r FreeBSD ${DEPS} ${NVIDIA_DEPS}
