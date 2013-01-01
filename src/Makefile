SOURCES = main.c NBT/*.c generate.c nbtsave.c
GCC = gcc
OUT = ../bin/out

CFLAGS = `pkg-config --cflags --libs gtk+-3.0` -std=c99 -g -Wall -s -Os -lm

all: $(OUT)

$(OUT): $(SOURCES)
	$(GCC) -o $@ $^ $(CFLAGS) -lz

