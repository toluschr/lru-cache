TOPDIR ?= $(or $(shell git rev-parse --show-superproject-working-tree),$(shell git rev-parse --show-toplevel))

CC := gcc
CFLAGS := -O3 -g0 -Wall -Wextra -Werror -pedantic -std=c11 -D_XOPEN_SOURCE=700
CFLAGS += -I include

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

.PHONY: all
all: lib/lru-cache.o

.PHONY: clean
clean:
	-rm -f lib/lru-cache.o test/lru-cache.o test/lru-cache

test/lru-cache: lib/lru-cache.o test/lru-cache.o
	$(CC) $^ $(LDFLAGS) -o $@


%.o: %.c include/lru-cache.h Makefile
	$(CC) $< $(CFLAGS) -c -o $@
