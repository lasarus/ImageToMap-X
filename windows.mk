CC := i486-mingw32-gcc
CFLAGS := -Wall -Werror -std=c99 -g -s -Os -mwindows `./pkg-config-script-win --cflags gtk+-2.0` -DGTK2
LFLAGS := `./pkg-config-script-win --libs gtk+-2.0` -lm -lz -mwindows

WINDRES := i486-mingw32-windres

OBJ := obj/windows/
BIN := bin/windows/

TARGET := $(BIN)ImageToMapX.exe

SRCDIRS := src src/* src/*/* src/*/*/*
SOURCES := $(foreach DIR, $(SRCDIRS), $(wildcard $(DIR)/*.c))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(SOURCES))

BEG = 	echo -e -n "  \033[32m$(1)$(2)...\033[0m" ; echo -n > /tmp/.`whoami`-build-errors
END = 	if [[ -s /tmp/.`whoami`-build-errors ]] ; then \
		echo -e -n "\r\033[1;33m$(1)$(2)\033[0m\n"; \
		cat /tmp/.`whoami`-build-errors; \
	else \
		echo -e -n "\r  \033[1;32m$(1)$(2)\033[0m\033[K\n"; \
	fi

INFO = echo -e -n "\r  \033[1;34m$(1)$(2)\033[0m\n"

ERRORFUNC = echo -e " \033[1;31m! Fatal error encountered.\033[0m"; \
	cat /tmp/.`whoami`-build-errors; \
	exit 1;

ERRORS = 2>>/tmp/.`whoami`-build-errors || { $(call ERRORFUNC) }

ERRORSS = >>/tmp/.`whoami`-build-errors || { $(call ERRORFUNC) }

BEGRM = echo -e -n "  \033[31m$(1)$(2)...\033[0m" && echo -n > /tmp/.`whoami`-build-errors
ENDRM = echo -e -n "\r  \033[1;31m$(1)$(2)\033[0m\033[K\n"


.PHONY: all clean init

all: init  $(TARGET)
	@$(call BEG, "CP", "win")
	@cp -r win/* $(BIN)
	@$(call END, "CP", "win")
	@$(call BEG, "CP", "resources")
	@cp -r resources/* $(BIN)
	@$(call END, "CP", "resources")
	@$(call INFO, "--", "Building ImageToMap-X for Windows...Done!");

init:
	@$(call INFO, "--", "Building ImageToMap-X for Windows...");

$(OBJ)%.o: %.c
	@$(call BEG, "CC", "$<")
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@ $(ERRORS)
	@$(call END, "CC", "$<")

$(TARGET): $(OBJECTS) $(OBJ)imagetomap.res
	@$(call BEG, "LD", "$<")
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) $(OBJ)imagetomap.res $(LFLAGS) -o $(TARGET) -s $(ERRORS)
	@$(call END, "LD", "$<")

$(OBJ)imagetomap.res: imagetomap.rc
	@$(call BEG, "WINDRES", "$<")
	@mkdir -p $(dir $@)
	@$(WINDRES) $< -O coff -o $@ $(ERRORS)
	@$(call END, "WINDRES", "$<")

clean:
	@$(RM) -rf $(BIN) $(OBJ)
