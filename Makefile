.PHONY: all clean linux windows

all: linux

clean:
	+rm -rf bin obj

linux:
	+$(MAKE) -f linux.mk

windows:
	+$(MAKE) -f windows.mk
