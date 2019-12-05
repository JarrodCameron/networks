# Author: Jarrod Cameron (z5210220)
# Date:   13/10/19 14:23

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=server client

CC=gcc
CFLAGS=-Wall -Wextra -Werror -I$(INCDIR)

LDFLAGS=-pthread

SERVER_DEPS= \
	connection.o \
	header.o \
	iter.o \
	list.o \
	logger.o \
	queue.o \
	server.o \
	slogin.o \
	status.o \
	synch.o \
	user.o \
	util.o

CLIENT_DEPS= \
	banner.o \
	client.o \
	clogin.o \
	header.o \
	iter.o \
	list.o \
	ptop.o \
	queue.o \
	status.o \
	synch.o \
	util.o

.PHONY: all clean

all: $(BUILDDIR) $(BINS)

client: $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))
	$(CC) $(LDFLAGS) -DI_AM_CLIENT -o client $(addprefix $(BUILDDIR)/, $(CLIENT_DEPS))

server: $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))
	$(CC) $(LDFLAGS) -DI_AM_SERVER -o server $(addprefix $(BUILDDIR)/, $(SERVER_DEPS))

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BINS) $(BUILDDIR)
