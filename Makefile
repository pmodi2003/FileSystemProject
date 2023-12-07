
SRCS := $(wildcard *.c) helpers/slist.c helpers/blocks.c helpers/bitmap.c
OBJS := $(SRCS:.c=.o)
HDRS := $(wildcard *.h) helpers/slist.h helpers/blocks.h helpers/bitmap.h

CFLAGS := -g `pkg-config fuse --cflags`
LDLIBS := `pkg-config fuse --libs`

nufs: $(OBJS)
	gcc $(CLFAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HDRS)
	gcc $(CFLAGS) -c -o $@ $<

clean: unmount
	rm -f nufs *.o test.log data.nufs helpers/*.o
	rmdir mnt || true

mount: nufs
	mkdir -p mnt || true
	./nufs -s -f mnt data.nufs

unmount:
	fusermount -u mnt || true

test: nufs
	perl test.pl

gdb: nufs
	mkdir -p mnt || true
	gdb --args ./nufs -s -f mnt data.nufs

.PHONY: clean mount unmount gdb

