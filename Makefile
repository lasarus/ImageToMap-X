.PHONY: all clean linux windows mac

all: linux

clean:
	@+$(MAKE) -f linux.mk clean
	@+$(MAKE) -f windows.mk clean
	@+$(MAKE) -f mac.mk clean

linux:
	@+$(MAKE) -f linux.mk

windows:
	@+$(MAKE) -f windows.mk

mac:
	@+$(MAKE) -f mac.mk
