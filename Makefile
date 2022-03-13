CC = gcc
CPPFLAGS = -D_DEFAULT_SOURCE
CPPFLAGSTEST = -D_DEFAULT_SOURCE -Isrc
CFLAGS = -Wall -Wextra -Werror -std=c99 -fPIC -fno-builtin
LDFLAGS = -shared
VPATH = src tests

TARGET_LIB = libmalloc.so
OBJS = malloc.o tools.o
TSRC = src/malloc.c src/tools.c tests/tests.c

all: library

library: $(TARGET_LIB)
$(TARGET_LIB): CFLAGS += -pedantic -fvisibility=hidden
$(TARGET_LIB): LDFLAGS += -Wl,--no-undefined
$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g
debug: clean $(TARGET_LIB)

check: library $(TSRC)
	$(CC) $(CPPFLAGSTEST) $(CFLAGS) $(TSRC) -o check -lcriterion
	./check

clean:
	$(RM) $(TARGET_LIB) $(OBJS) check

.PHONY: all $(TARGET_LIB) clean
