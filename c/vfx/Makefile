TARGET = lib/vfx.ffi

#LOC = -DASMV
#LOC = -DDEBUG -g

# -ffast-math 

INCLUDE = -I../../runtime/ -I./h -I../gfx/src -I../../ncc/h/ -I../../ncu/
PREFIX =
CC = $(PREFIX)gcc
CFLAGS = $(LOC) $(DEPS) -O3 -mpopcnt -ffast-math $(INCLUDE) -DBZ_STRICT_ANSI --pedantic
EXTRA_CFLAGS = 

AS = $(CC)
ASFLAGS = $(LOC) -Wall

LD = $(CC)
LDFLAGS = $(LOC)
AR = $(PREFIX)ar
ARFLAGS = rcs

STRIP = $(PREFIX)strip
CP = cp -fp
RM = rm -f

#SRCS = main.c
SRCS = slb.c svo.c render.c texture.c gfx_primitives.c file.c \
       camera.c edit.c voxelize.c polygonize.c mesh.c imp_ply.c \
       fast_obj.c imp_obj.c imp_kvx.c imp_vxl.c nc.c ncu.c


HDRSB = common.h svo.h slb.h \
        edit.h camera.h gfx_primitives.h \
        sdf.h kdtree.h kdtree_median.h profile.h

HDRS = $(addprefix src/,$(HDRSB))

OBJS = $(addprefix lib/,$(SRCS:.c=.o))
OBJA =

#$(info $$OBJS is [${OBJS}])


all: $(TARGET)

lib/dummy:
	mkdir lib
	echo "dummy" >lib/dummy

src/%.h:
	@echo "header \"$@\" is missing"
	@exit -1

# pp=$@.c.i

lib/%.o: src/%.c $(HDRS)
	@echo NCC $<
	@../../ncc/lib/ncc i=../../ncc/h/ $< $@.c
	@echo CC $@.c
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $@.c

$(TARGET): lib/dummy $(OBJS) $(OBJA)
	@echo LINK $@
	@$(LD) -fpic -shared -o $@ $(OBJS) $(OBJA) $(LDFLAGS)

lib/util: lib/dummy $(OBJS) lib/util.o $(OBJA)
	$(LD) -o $@ $(OBJS) lib/util.o $(OBJA) $(LDFLAGS)

.PHONY: install uninstall clean

clean:
	-$(RM) $(TARGET)
	-$(RM) *.o
	-$(RM) *.exe


