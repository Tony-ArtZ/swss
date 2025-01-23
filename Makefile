CC = gcc
CFLAGS = -fPIC -Wall -Wextra -I./include
LDFLAGS = -shared
LIBS = -lssl -lcrypto -lpthread

# Installation directories
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include/swss

# Library source files
SRC = src/swss.c src/utils.c src/base64.c
OBJ = $(SRC:.c=.o)
LIB = libswss.so

# Example program
EXAMPLE_SRC = example/main.c
EXAMPLE_BIN = example/chat_server

all: $(LIB) $(EXAMPLE_BIN)

# Build shared library
$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build example program
$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB)
	$(CC) $(CFLAGS) -I$(PWD)/include -o $@ $< -L. -lswss $(LIBS)

# Install the library and headers
install: $(LIB)
	install -d $(INCLUDEDIR)
	install -m 644 include/*.h $(INCLUDEDIR)
	install -m 755 $(LIB) $(LIBDIR)
	ldconfig

# Uninstall the library and headers
uninstall:
	rm -rf $(INCLUDEDIR)
	rm -f $(LIBDIR)/$(LIB)
	ldconfig

# Clean build files
clean:
	rm -f $(OBJ) $(LIB) $(EXAMPLE_BIN)

.PHONY: all install uninstall clean
