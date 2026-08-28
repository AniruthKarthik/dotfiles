CC ?= gcc
CFLAGS ?= -O2
PKGS = gtk-layer-shell-0 gtk+-3.0
CFLAGS_PKGS := $(shell pkg-config --cflags $(PKGS))
LIBS := $(shell pkg-config --libs $(PKGS))

TARGET = hypr-edge-switcher
SRC = hypr-edge-switcher.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CFLAGS_PKGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
