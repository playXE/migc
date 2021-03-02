CC=clang
CXX=clang++
CURRENT_DIR=$(shell pwd)
MALLOC_LIBDIR=$(CURRENT_DIR)/mimalloc/
CFLAGS=-O3 
LDFLAGS=-L$(TBB_LIBDIR) -Wl,-rpath=$(TBB_LIBDIR) \
        -L$(MALLOC_LIBDIR) -Wl,-rpath=$(MALLOC_LIBDIR)
LIBS=-pthread -lmimalloc
OBJS=migc.o

libmigc.so: $(OBJS)
	$(CC) -shared $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

$(OBJS): migc.c migc.h Makefile
submodules: 
	mkdir -p mimalloc/out/release
	(cd mimalloc/out/release; cmake ../..)
	$(MAKE) -C mimalloc/

clean:
	rm -f *.0 *~ migc