CC := gcc
CFLAGS := -O3 -Wall -Wextra -Werror -pedantic -std=c11 -D_XOPEN_SOURCE=700

.PHONY: all
all: lru-cache lru-cache.o

.PHONY: clean
clean:
	-rm -f lru-cache lru-cache.o test.o

lru-cache: lru-cache.o test.o
	$(CC) $(CFLAGS) $^ -o $@

test.o: test.c lru-cache.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

lru-cache.o: lru-cache.c lru-cache.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@
