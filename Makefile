CC = gcc
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -Wall -Wextra -Werror -std=c99 -fPIC -fno-builtin
LDFLAGS = -shared
VPATH = src

TARGET_LIB = libmalloc.so
OBJS = malloc.o tools.o

all: library

library: $(TARGET_LIB)
$(TARGET_LIB): CFLAGS += -pedantic -fvisibility=hidden
$(TARGET_LIB): LDFLAGS += -Wl,--no-undefined
$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g
debug: clean $(TARGET_LIB)

clean:
	$(RM) $(TARGET_LIB) $(OBJS)

.PHONY: all $(TARGET_LIB) clean
