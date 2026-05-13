# Generic POSIX build of cffilib (Linux, BSD).
# Output: lib/cffi.ffi — a renamed shared object.

TARGET    = lib/cffi.ffi
CC       ?= cc
CFLAGS   ?= -O2 -Wall -Wextra -Wno-unused-parameter \
            -fvisibility=hidden -fPIC
LDFLAGS  ?= -shared

SRCS = src/cffilib.c
OBJS = $(SRCS:src/%.c=lib/%.o)

all: $(TARGET)

lib:
	mkdir -p lib

lib/%.o: src/%.c | lib
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(TARGET)
