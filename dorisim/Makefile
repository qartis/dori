IDIRS=-I../shared/include/
CC=gcc
CCFLAGS=-g -Wall -Wextra $(IDIRS)
LDFLAGS= -lpthread
SRCDIR=./src

vpath %.c ./src

all: dorisim

dorisim: dorisim.c
	$(CC) $(CCFLAGS) $(SRCDIR)/dorisim.c -o dorisim $(LDFLAGS)

clean:
	rm dorisim
