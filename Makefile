# Simple -*- Makefile -*- for ring_buffer
#
BASEFLAGS=-O2 -Wall -Wextra -Werror -DHAVE_INT128=1

DEPS=Makefile ring_buffer.h
OBJS=rb_test.o ring_buffer.o
EXE=rb_test
LDFLAGS=-flto

# Verify compilation with c89 and c++
#
ifeq ($(CVER),c89)
CFLAGS=$(BASEFLAGS) -std=c89 -Dinline="static __inline" -D_GNU_SOURCE -flto
else ifeq ($(CVER),c++)
CFLAGS=$(BASEFLAGS)
CC=$(CXX)
CFLAGS += -x c++
LDFLAGS += -nodefaultlibs -lc
else
CFLAGS=$(BASEFLAGS)
endif

all: $(EXE) $(DEPS)

test: all
	./rb_test

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *~ $(OBJS)

distclean: clean
	rm -f $(EXE)
