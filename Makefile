CC := gcc
CFLAGS := -ggdb -O0 -Wall -Wextra -Werror -pedantic -std=c11 -D_XOPEN_SOURCE=700

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

BIN = test benchmark

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

.PHONY: all
all: $(BIN) lru-cache.o

.PHONY: clean
clean:
	-rm -f $(BIN) lru-cache.o test.o benchmark.o

$(BIN): %: %.o lru-cache.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c lru-cache.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@
