INCDIRS=../shared/include ../
CC=gcc
CFLAGS=-g -Wall -Wextra -Wshadow $(foreach DIR,$(INCDIRS), -I$(DIR)) -Wno-override-init
LDFLAGS=-lsqlite3
SRCDIR=./src

all: gateway

gateway: $(SRCDIR)/gateway.c
	$(CC) $(CFLAGS)  $(SRCDIR)/gateway.c -o gateway $(LDFLAGS)

clean:
	rm gateway
