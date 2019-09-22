BUILD_DIR ?= . # ${.CURDIR}
PREFIX    ?= /opt

PROJECT  = steam-utils
CFLAGS   = -std=c99 -Wall -Wextra -Wno-unused-parameter -fPIC -shared --sysroot=/compat/linux
LIBS    := lib32/steamfix.so lib32/libnm-glib.so.4 lib32/libpulse.so.0 lib64/webfix.so
LIBS    := ${LIBS:C|(.*)|$(BUILD_DIR)/\1|}

build: $(LIBS)

.for b in 32 64

$(BUILD_DIR)/lib$(b)/steamfix.so: src/steamfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -o $(.TARGET) src/steamfix.c -ldl

$(BUILD_DIR)/lib$(b)/webfix.so: src/webfix.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -o $(.TARGET) src/webfix.c   -ldl

$(BUILD_DIR)/lib$(b)/libnm-glib.so.4: src/fakenm.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -o $(.TARGET) src/fakenm.c

$(BUILD_DIR)/lib$(b)/libpulse.so.0: src/fakepulse.c
	mkdir -p $(BUILD_DIR)/lib$(b)
	/compat/linux/bin/cc -m$(b) $(CFLAGS) -o $(.TARGET) src/fakepulse.c

.endfor

clean:
.for f in $(LIBS)
.  if exists($f)
	rm $f
.  endif
.endfor

install: build
	install -d $(PREFIX)/$(PROJECT)
	install -d $(PREFIX)/$(PROJECT)/bin
	install -d $(PREFIX)/$(PROJECT)/lxbin
	install -d $(PREFIX)/$(PROJECT)/lib32
	install -d $(PREFIX)/$(PROJECT)/lib64
	install bin/*   $(PREFIX)/$(PROJECT)/bin
	install lxbin/* $(PREFIX)/$(PROJECT)/lxbin
	install $(BUILD_DIR)/lib32/* $(PREFIX)/$(PROJECT)/lib32
#	install $(BUILD_DIR)/lib64/* $(PREFIX)/$(PROJECT)/lib64

deinstall:
.if exists($(PREFIX)/$(PROJECT))
	rm -r -I $(PREFIX)/$(PROJECT)
.endif
