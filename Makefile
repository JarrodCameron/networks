# Author: Jarrod Cameron (z5210220)
# Date:   13/10/19 14:23

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=server client

CC=gcc
CFLAGS=-Wall -Wextra -ggdb -I$(INCDIR) -lpthread

SERVER_DEPS=server.o user.o list.o
CLIENT_DEPS=client.o

.PHONY: all clean

all: $(BUILDDIR) client server

client: $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))
	$(CC) -o client $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))

server: $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))
	$(CC) -o server $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf *.o $(BINS) $(BUILDDIR) cscope.out tags
