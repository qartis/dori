IDIRS=-I./include/ -I./data -I/usr/include
CC=g++
CFLAGS=-Wall -Wextra $(IDIRS)
LDFLAGS=
SRCDIR=./src
OBJDIR=./obj

vpath %.cxx ./src
vpath %.cpp ./src
vpath %.h ./include
vpath %.H ./include

libmultiarcball.so: $(OBJDIR)/basic_ball.o $(OBJDIR)/3dcontrols.o $(OBJDIR)/Fl_Custom_Cursor.o $(OBJDIR)/FlGlArcballWindow.o 
	$(CC) $(CFLAGS) $(OBJDIR)/basic_ball.o $(OBJDIR)/3dcontrols.o $(OBJDIR)/Fl_Custom_Cursor.o $(OBJDIR)/FlGlArcballWindow.o -shared -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp %.h
	$(CC) $(CFLAGS) -c $< -fPIC -o $@

#for Fl_Custom_Cursor
$(OBJDIR)/%.o: %.cxx %.H
	$(CC) $(CFLAGS) -c $< -fPIC -o $@

clean:
	rm $(OBJDIR)/*.o libmultiarcball.so
