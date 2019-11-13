# Author: Jarrod Cameron (z5210220)
# Date:   13/10/19 14:23

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=server client

CC=gcc
CFLAGS=-Wall -Wextra -g3 -ggdb -I$(INCDIR)

LDFLAGS=-pthread

SERVER_DEPS=server.o user.o list.o header.o slogin.o logger.o util.o synch.o status.o iter.o
CLIENT_DEPS=client.o clogin.o header.o util.o status.o banner.o queue.o synch.o iter.o

.PHONY: all clean run full

all: $(BUILDDIR) $(BINS)

client: $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))
	$(CC) $(LDFLAGS) -o client $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))

server: $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))
	$(CC) $(LDFLAGS) -o server $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	bash ./run.sh

full: clean all

clean:
	rm -rf $(BINS) $(BUILDDIR) vgcore.*
