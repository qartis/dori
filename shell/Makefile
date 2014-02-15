INCDIRS = ../shared/include/ ./include/ ../
CC=gcc
CFLAGS=-g3 -Wno-write-strings -Wall -Wextra $(foreach DIR,$(INCDIRS), -I$(DIR))  -Wno-override-init
LDFLAGS=
SRCDIR=./src
INCLUDES = $(wildcard $(foreach DIR,$(INCDIRS), $(DIR)/*))
SRCS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))
OBJS = $(patsubst src/%,obj/%,$(SRCS))

vpath %.c ./src
vpath %.h ./include

all: send logger

send: send.c ../include/shell.h
	$(CC) $(CFLAGS) $< -o $@

dcim_read: dcim_read.c
	$(CC) $(CFLAGS) $< -o $@

logger: logger.c

clean:
	rm send
