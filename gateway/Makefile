INCDIRS=../shared/include ../
CC=gcc
CFLAGS=-g -Wall -Wextra $(foreach DIR,$(INCDIRS), -I$(DIR)) -Wno-override-init
LDFLAGS=-lsqlite3
SRCDIR=./src

all: gateway.o

gateway.o: $(SRCDIR)/gateway.c
	$(CC) $(CFLAGS)  $(SRCDIR)/gateway.c -o gateway $(LDFLAGS)
	sudo chown root:root gateway
	sudo chmod +s gateway

clean:
	rm gateway
