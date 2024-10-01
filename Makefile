CC := gcc

CFLAGS += -Og
CFLAGS += -ggdb3

.PHONY: all
all: lru-cache

lru-cache: lru-cache.o test.o
	$(CC) $(CFLAGS) $^ -o $@

test.o: test.c lru-cache.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

lru-cache.o: lru-cache.c lru-cache.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@
