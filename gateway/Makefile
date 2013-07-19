IDIR=../shared/include
CC=gcc
CFLAGS=-g -Wall -Wextra -I$(IDIR)
LDFLAGS=-lsqlite3
SRCDIR=./src

all: gateway.o shell_monitor

gateway.o: $(SRCDIR)/gateway.c
	$(CC) $(CFLAGS)  $(SRCDIR)/gateway.c -o gateway $(LDFLAGS)
	sudo chown root:root gateway
	sudo chmod +s gateway

shell_monitor:
	mkfifo shell_monitor

clean:
	rm gateway shell_monitor
