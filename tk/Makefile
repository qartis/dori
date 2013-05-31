INCDIRS = ../shared/include/ ./include/ ../multiarcball/include ../widgets/include
#CXX=g++
CXXFLAGS=-g -Wno-write-strings -Wall -Wextra $(foreach DIR,$(INCDIRS), -I$(DIR))
LDFLAGS=-L../multiarcball -L../widgets `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -lwidgets -lmultiarcball -lglut -lGLU -L/opt/X11/lib -lXcursor
SRCDIR=./src
BUILDDIR=./obj
INCLUDES = $(wildcard $(foreach DIR,$(INCDIRS), $(DIR)/*))
SRCS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))
OBJS = $(patsubst src/%,obj/%,$(SRCS))

vpath %.cpp ./src
vpath %.h ./include

tk: $(OBJS) $(INCLUDES)
	$(CXX) $(CXXFLAGS) $(OBJS) -o tk $(LDFLAGS)

$(BUILDDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm $(BUILDDIR)/*.o tk

run:
	LD_LIBRARY_PATH="../widgets:../multiarcball" ./tk
