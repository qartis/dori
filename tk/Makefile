IDIRS=-I../shared/include/ -I./include/ -I../multiarcball/include
CXX=g++
CXXFLAGS=-Wall -Wextra $(IDIRS)
LDFLAGS=-L../multiarcball `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -lmultiarcball -lglut -lGLU -L/opt/X11/lib -lXcursor
SRCDIR=./src
BUILDDIR=./obj

vpath %.cpp ./src
vpath %.h ./include

tk: $(BUILDDIR)/radarwindow.o $(BUILDDIR)/viewport.o $(BUILDDIR)/siteeditor.o $(BUILDDIR)/rectobject.o $(BUILDDIR)/circleobject.o $(BUILDDIR)/lineobject.o $(BUILDDIR)/toolbar.o $(BUILDDIR)/objmodel.o $(BUILDDIR)/mainwindow.o $(BUILDDIR)/table.o $(BUILDDIR)/queryinput.o $(BUILDDIR)/widgetwindow.o
	$(CXX) $(CXXFLAGS) $(BUILDDIR)/*.o $(SRCDIR)/main.cpp -o tk $(LDFLAGS); export LD_LIBRARY_PATH="../multiarcball"

$(BUILDDIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm $(BUILDDIR)/*.o tk
