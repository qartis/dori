INCDIRS = ../shared/include/ ./include/ ../
CXX=gcc
CXXFLAGS=-g -Wno-write-strings -Wall -Wextra $(foreach DIR,$(INCDIRS), -I$(DIR))  -Wno-override-init
LDFLAGS=
SRCDIR=./src
INCLUDES = $(wildcard $(foreach DIR,$(INCDIRS), $(DIR)/*))
SRCS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))
OBJS = $(patsubst src/%,obj/%,$(SRCS))

vpath %.c ./src
vpath %.h ./include

all: send

send: send.c
	$(CXX) $(CXXFLAGS) $< -o $@

dcim_read: dcim_read.c
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm send
