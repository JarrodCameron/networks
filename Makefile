# Author: Jarrod Cameron (z5210220)
# Date:   13/10/19 14:23

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=server client

CC=gcc
CFLAGS=-Wall -Wextra -ggdb -I$(INCDIR)

LDFLAGS=-pthread

SERVER_DEPS=server.o user.o list.o header.o slogin.o
CLIENT_DEPS=client.o clogin.o

.PHONY: all clean

all: $(BUILDDIR) client server

client: $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))
	$(CC) $(LDFLAGS) -o client $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))

server: $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))
	$(CC) $(LDFLAGS) -o server $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf *.o $(BINS) $(BUILDDIR) cscope.out tags
