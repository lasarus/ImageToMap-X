CC := i686-apple-darwin10-gcc
CFLAGS := -Wall -Werror -std=c99 -g -s -Os `./pkg-config-script-mac --cflags gtk+-2.0` -DGTK2
LFLAGS := `./pkg-config-script-mac --libs gtk+-2.0` -lm -lz

OBJ := obj/mac/
BIN := bin/mac/

TARGET := $(BIN)ImageToMapX
APP := $(TARGET).app/Contents/

SRCDIRS := src src/* src/*/* src/*/*/*
SOURCES := $(foreach DIR, $(SRCDIRS), $(wildcard $(DIR)/*.c))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(SOURCES))

.PHONY: all

all: $(TARGET)
	mkdir -p $(APP){MacOS,Resources}
	cp $(TARGET) $(APP)MacOS/ImageToMap
	cp /usr/i686-apple-darwin10/lib/*.dylib $(APP)MacOS/
	cp -r resources/* $(APP)Resources/
	cp mac/Info.plist mac/PkgInfo $(APP)
	chmod -R 755 $(APP)

$(OBJ)%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN)
	$(CC) $(OBJECTS) $(LFLAGS) -o $(TARGET)
