.PHONY: all clean linux windows mac

all: linux windows mac

clean:
	+rm -rf bin obj
	
linux:
	+$(MAKE) -f linux.mk

windows:
	+$(MAKE) -f windows.mk

mac:
	+$(MAKE) -f mac.mk
