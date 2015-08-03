-include config.mk

PROG    ?= thesis
GCC     ?= gcc
CFLAGS  ?= -g -Wall -Werror --std=c99
LDFLAGS ?= -lgnutls

all: $(PROG)

clean:
	rm -f $(PROG) *.o

$(PROG): main.o peer.o poll.o sock.o
	$(GCC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c $(wildcard *.h) makefile
	$(GCC) $(CFLAGS) -c -o $@ $<
