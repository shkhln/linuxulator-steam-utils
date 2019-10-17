.if !defined(OSVERSION)
OSVERSION != awk '/^\#define[[:blank:]]__FreeBSD_version/ {print $$3}' < /usr/include/sys/param.h # from bsd.port.mk
.endif

PROJECT = steam-utils

BUILD_DIR ?= .
PREFIX    ?= /opt

CFLAGS = --sysroot=/compat/linux -std=c99 -Wall -Wextra -Wno-unused-parameter

.if $(OSVERSION) > 1300053
CFLAGS += -DSKIP_FUTEX_WORKAROUND
.endif

LIBS  = lib32/steamfix.so lib32/libnm-glib.so.4 lib32/libpulse.so.0 lib64/webfix.so
LIBS := ${LIBS:C|(.*)|$(BUILD_DIR)/\1|}

BINS  = lxbin/fhelper32 lxbin/fhelper64
BINS := ${BINS:C|(.*)|$(BUILD_DIR)/\1|}

build: $(LIBS) $(BINS)

.for b in 32 64

$(BUILD_DIR)/lib$(b)/steamfix.so: src/steamfix.c src/futexes.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/steamfix.c src/futexes.c -pthread -ldl

$(BUILD_DIR)/lib$(b)/webfix.so: src/webfix.c src/futexes.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/webfix.c   src/futexes.c -pthread -ldl

$(BUILD_DIR)/lib$(b)/libnm-glib.so.4: src/fakenm.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/fakenm.c

$(BUILD_DIR)/lib$(b)/libpulse.so.0: src/fakepulse.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -fPIC -shared -o $(.TARGET) src/fakepulse.c

$(BUILD_DIR)/lxbin/fhelper$(b): src/futex_helper.c
	mkdir -p $(BUILD_DIR)/lxbin
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -o $(.TARGET) src/futex_helper.c

.endfor

clean:
.for f in $(LIBS) $(BINS)
.  if exists($f)
	rm $f
.  endif
.endfor

install:
	install -d $(PREFIX)/$(PROJECT)
.for d in bin lxbin lib32 lib64
	install -d $(PREFIX)/$(PROJECT)/$(d)
.  if exists($d)
	install $(d)/* $(PREFIX)/$(PROJECT)/$(d)
.  endif
.  if $(BUILD_DIR) != "."
.    if exists($BUILD_DIR/$d)
	install $(BUILD_DIR)/$(d)/* $(PREFIX)/$(PROJECT)/$(d)
.    endif
.  endif
.endfor

deinstall:
.if exists($(PREFIX)/$(PROJECT))
	rm -r -I $(PREFIX)/$(PROJECT)
.endif
