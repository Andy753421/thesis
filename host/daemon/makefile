-include config.mk

PROG     ?= thesis
GCC      ?= gcc
CFLAGS   ?= -g -Wall -Werror --std=c99

CPPFLAGS += -DUSE_OPENSSL
LDFLAGS  += -lcrypto

all: $(PROG)

clean:
	rm -f $(PROG) *.o

$(PROG): main.o util.o bc.o mc.o tcp.o web.o http.o ws.o
	$(GCC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c $(wildcard *.h) makefile
	$(GCC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
