CFLAGS=-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -O3  -pedantic -std=c11
CC=cc
LIBS=

EXAMPLES=$(patsubst %.basm, %.bm, $(wildcard ls ./examples/*.basm))


.PHONY: all
all: basm bme debasm

basm: ./src/basm.c ./src/bm.h
	$(CC) $(CFLAGS) -o basm ./src/basm.c $(LIBS)

bme: ./src/bme.c ./src/bm.h
	$(CC) $(CFLAGS) -o bme ./src/bme.c $(LIBS)

debasm: ./src/debasm.c ./src/bm.h
	$(CC) $(CFLAGS) -o debasm ./src/debasm.c $(LIBS)


.PHONY: examples
examples: $(EXAMPLES)

%.bm: %.basm basm
		./basm $< $@ 