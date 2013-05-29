IDIRS=-I./include/ -I./data -I/usr/include
CC=g++
CFLAGS=-g -Wall -Wextra $(IDIRS)
LDFLAGS=`fltk-config --use-gl --cxxflags --ldflags` -lcurl -lglut -lGLU -L/opt/X11/lib -lXcursor -lfltk_images
SRCDIR=./src
OBJDIR=./obj

vpath %.cxx ./src
vpath %.cpp ./src
vpath %.h ./include
vpath %.H ./include

libwidgets.so: $(OBJDIR)/sparkline.o $(OBJDIR)/fl_gauge.o $(OBJDIR)/fl_compass.o $(OBJDIR)/maptile.o
	$(CC) $(CFLAGS) $(OBJDIR)/sparkline.o $(OBJDIR)/fl_gauge.o $(OBJDIR)/fl_compass.o $(OBJDIR)/maptile.o $(LDFLAGS) -shared -o $@

$(OBJDIR)/%.o: %.cpp %.h
	$(CC) $(CFLAGS) -c $< -fPIC -o $@

$(OBJDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -fPIC -o $@

clean:
	rm $(OBJDIR)/*.o libwidgets.so
