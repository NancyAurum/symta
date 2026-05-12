TARGET = cinvoke_lua.dylib

all: $(TARGET)

test:
	lua test.lua

clean:
	rm -f *.o $(TARGET)

SRCS = cinvoke_lua.c

OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	libtool -dynamic -lc -o $(TARGET) $(OBJS) -L../../lib -lcinvoke

.c.o:
	gcc -g -Wall -DCINVOKE_BUILD -DARCH_GCC_X64_OSX -dynamic -I../../lib -c $< -o $@

