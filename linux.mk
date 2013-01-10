CC := gcc
CFLAGS := -Wall -Werror -std=c99 -g -s -Os `pkg-config --cflags gtk+-3.0`
LFLAGS := `pkg-config --libs gtk+-3.0` -lm -lz

OBJ := obj/linux/
BIN := bin/linux/

TARGET := $(BIN)ImageToMapX

SRCDIRS := src src/* src/*/* src/*/*/*
SOURCES := $(foreach DIR, $(SRCDIRS), $(wildcard $(DIR)/*.c))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(SOURCES))

.PHONY: all

all: $(TARGET)
	cp -r resources/* $(BIN)

$(OBJ)%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN)
	$(CC) $(LFLAGS) $(OBJECTS) -o $(TARGET) -s
