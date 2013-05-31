INCDIRS = ../shared/include/ ./include/ ../multiarcball/include ../widgets/include
#CXX=g++
CXXFLAGS=-g -Wno-write-strings -Wall -Wextra $(foreach DIR,$(INCLUDES), -I$(DIR))
LDFLAGS=-L../multiarcball -L../widgets `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -lwidgets -lmultiarcball -lglut -lGLU -L/opt/X11/lib -lXcursor
SRCDIR=./src
BUILDDIR=./obj
INCLUDES = $(wildcard $(foreach DIR,$(INCDIRS), $(DIR)/*))
OBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

vpath %.cpp ./src
vpath %.h ./include

tk: $(SRCDIR)/main.cpp $(OBJS) $(INCLUDES)
	$(CXX) $(CXXFLAGS) $(BUILDDIR)/*.o $(SRCDIR)/main.cpp -o tk $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm $(BUILDDIR)/*.o tk

run:
	LD_LIBRARY_PATH="../widgets:../multiarcball" ./tk
