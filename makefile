
SHELL = /bin/sh
CC = g++
LIBS = -lpthread
AR = ar
RANLIB = ranlib
SRCDIR = .
ARFLAGS = crs
CFLAGS += -D _DEBUG
CFLAGS += -g 
INCLUDES += ./
BIN := a.out
STATICLIB = libhello.a
SHAREDLIB = libhello.so

SRCS = $(wildcard $(SRCDIR)/*.c++)
OBJS = $(patsubst %.c++,%.o,$(SRCS))

.PHONY: all clean help staticlib

all: $(BIN) 

# build static lib
staticlib: $(objs) $(STATICLIB)
$(STATICLIB):$(OBJS)
	$(AR) $(ARFLAGS) $@ $^

#build shared lib
sharedlib: $(objs) $(SHAREDLIB)
$(SHAREDLIB):$(OBJS)
	$(CC) -shared -o $@ $^

$(BIN):$(OBJS)
	$(CC)  -o $@  $^ $(LIBS)

%.o:%.c++ 
	$(CC) $(CFLAGS) -I $(INCLUDES) -c $< -o $@

help:
	@echo ""
	@echo "make all"
	@echo "make clean"
	@echo "make staticlib"
	@echo "make sharedlib"
	@echo ""
clean:
	-rm -rf $(OBJS)  *.a *.so $(BIN) 
