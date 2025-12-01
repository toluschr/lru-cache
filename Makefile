CC := gcc
CFLAGS := -ggdb -O0 -Wall -Wextra -Werror -pedantic -std=c11 -D_XOPEN_SOURCE=700

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

BIN = test benchmark

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

.PHONY: all
all: $(BIN) cachemap.o

.PHONY: clean
clean:
	-rm -f $(BIN) cachemap.o test.o benchmark.o

$(BIN): %: %.o cachemap.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c cachemap.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@
