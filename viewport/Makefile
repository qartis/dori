all: 3dcontrols.o basic_ball.o FlGlArcballWindow.o Fl_Custom_Cursor.o viewport.o

3dcontrols.o: ../multiarcball/src/math/3dcontrols.cpp
	g++ -g -c ../multiarcball/src/math/3dcontrols.cpp -I../multiarcball/src/math

basic_ball.o: ../multiarcball/src/math/basic_ball.cpp
	g++ -g -c ../multiarcball/src/math/basic_ball.cpp -I../multiarcball/src/math

FlGlArcballWindow.o: ../multiarcball/src/FLui/FlGlArcballWindow.cpp
	g++ -g -c ../multiarcball/src/FLui/FlGlArcballWindow.cpp -I../multiarcball/src/math/

Fl_Custom_Cursor.o: ../multiarcball/src/flcursors/fltk1/Fl_Custom_Cursor.cxx
	g++ -g -c ../multiarcball/src/flcursors/fltk1/Fl_Custom_Cursor.cxx

viewport.o: viewport.cpp objmodel.cpp 3dcontrols.o basic_ball.o FlGlArcballWindow.o Fl_Custom_Cursor.o ../siteeditor/lineobject.o ../siteeditor/rectobject.o ../siteeditor/circleobject.o
	g++ -g -c -O3 objmodel.cpp viewport.cpp -I/opt/X11/include -I../ -I../multiarcball/src/FLui -I../multiarcball/src/math -I../multiarcball/src/flcursors/fltk1 -I../mathgl/include/ -I../siteeditor/ `fltk-config --use-gl --cxxflags --ldflags` -L/opt/X11/lib -lglut -lGLU -lmgl -lmgl-fltk -lmgl-wnd

clean:
	rm *.o
