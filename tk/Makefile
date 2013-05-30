IDIRS=-I../shared/include/ -I./include/ -I../multiarcball/include -I../widgets/include
CXX=g++
CXXFLAGS=-Wno-write-strings -Wall -Wextra $(IDIRS) -g
LDFLAGS=-L../multiarcball -L../widgets `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -lwidgets -lmultiarcball -lglut -lGLU -L/opt/X11/lib -lXcursor
SRCDIR=./src
BUILDDIR=./obj

vpath %.cpp ./src
vpath %.h ./include

tk: $(SRCDIR)/main.cpp $(BUILDDIR)/graph.o $(BUILDDIR)/radarwindow.o $(BUILDDIR)/viewport.o $(BUILDDIR)/siteeditor.o $(BUILDDIR)/rectobject.o $(BUILDDIR)/circleobject.o $(BUILDDIR)/lineobject.o $(BUILDDIR)/toolbar.o $(BUILDDIR)/objmodel.o $(BUILDDIR)/mainwindow.o $(BUILDDIR)/table.o $(BUILDDIR)/queryinput.o $(BUILDDIR)/widgetwindow.o
	$(CXX) $(CXXFLAGS) $(BUILDDIR)/*.o $(SRCDIR)/main.cpp -o tk $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm $(BUILDDIR)/*.o tk

run:
	LD_LIBRARY_PATH="../widgets:../multiarcball" ./tk
