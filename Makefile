.PHONY: all clean linux windows

all: linux

clean:
	@+$(MAKE) -f linux.mk clean
	@+$(MAKE) -f windows.mk clean

linux:
	@+$(MAKE) -f linux.mk

windows:
	@+$(MAKE) -f windows.mk
