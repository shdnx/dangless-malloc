CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -Iinclude -I../ix-dune/libdune

all: test-dune

test-dune: src/*.c include/*.h
	$(CC) $(CFLAGS) $^ -o $@ -L../ix-dune/libdune -ldune
