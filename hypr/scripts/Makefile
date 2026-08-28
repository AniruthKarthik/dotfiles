CC ?= gcc
CFLAGS ?= -O2 -Wall
WAYLAND_SCANNER = wayland-scanner

PKGS = wayland-client
CFLAGS_PKGS := $(shell pkg-config --cflags $(PKGS))
LIBS := $(shell pkg-config --libs $(PKGS))

TARGET = hypr-edge-switcher
SRC = hypr-edge-switcher.c wlr-layer-shell-unstable-v1-protocol.c
HDR = wlr-layer-shell-unstable-v1-client-protocol.h

all: $(TARGET)

$(HDR): wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) client-header $< $@

wlr-layer-shell-unstable-v1-protocol.c: wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) private-code $< $@

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(CFLAGS_PKGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) wlr-layer-shell-unstable-v1-client-protocol.h wlr-layer-shell-unstable-v1-protocol.c

.PHONY: all clean
