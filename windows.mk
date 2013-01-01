CC := i486-mingw32-gcc
CFLAGS := -Wall -Werror -std=c99 -g -s -Os -mwindows `./pkg-config-script-win --cflags gtk+-2.0` -DGTK2
LFLAGS := `./pkg-config-script-win --libs gtk+-2.0` -lm -lz -mwindows

WINDRES := i486-mingw32-windres

OBJ := obj/windows/
BIN := bin/windows/

TARGET := $(BIN)ImageToMap.exe

SRCDIRS := src src/* src/*/* src/*/*/*
SOURCES := $(foreach DIR, $(SRCDIRS), $(wildcard $(DIR)/*.c))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(SOURCES))

.PHONY: all

all: $(TARGET)
	cp -r win/* $(BIN)
	cp -r resources/* $(BIN)

$(OBJ)%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(OBJ)imagetomap.res
	@mkdir -p $(BIN)
	$(CC) $(OBJECTS) $(OBJ)imagetomap.res $(LFLAGS) -o $(TARGET)

$(OBJ)imagetomap.res: imagetomap.rc
	$(WINDRES) $< -O coff -o $@
