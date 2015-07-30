-include config.mk

PROG   ?= thesis
GCC    ?= gcc
CFLAGS ?= -g -Wall --std=c99

all: $(PROG)

clean:
	rm -f $(PROG) *.o

$(PROG): main.o
	$(GCC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c $(wildcard *.h) makefile
	$(GCC) $(CFLAGS) -c -o $@ $<
