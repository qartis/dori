all: 3dcontrols.o basic_ball.o FlGlArcballWindow.o Fl_Custom_Cursor.o fl_contour.o fl_gl_contour.o fl_palette.o viewport.o

3dcontrols.o: ./multiarcball/src/math/3dcontrols.cpp
	g++ -g -c ./multiarcball/src/math/3dcontrols.cpp -I./multiarcball/src/math

basic_ball.o: ./multiarcball/src/math/basic_ball.cpp
	g++ -g -c ./multiarcball/src/math/basic_ball.cpp -I./multiarcball/src/math

FlGlArcballWindow.o: ./multiarcball/src/FLui/FlGlArcballWindow.cpp
	g++ -g -c ./multiarcball/src/FLui/FlGlArcballWindow.cpp -I./multiarcball/src/math/

Fl_Custom_Cursor.o: ./multiarcball/src/flcursors/fltk1/Fl_Custom_Cursor.cxx
	g++ -g -c ./multiarcball/src/flcursors/fltk1/Fl_Custom_Cursor.cxx

fl_contour.o: ./fltk_contour/src/fl_contour.cxx
	g++ -g -c ./fltk_contour/src/fl_contour.cxx -I./fltk_contour/include

fl_gl_contour.o: ./fltk_contour/src/fl_gl_contour.cxx
	g++ -g -c ./fltk_contour/src/fl_gl_contour.cxx -I./fltk_contour/include

fl_palette.o: ./fltk_contour/src/fl_palette.cxx
	g++ -g -c ./fltk_contour/src/fl_palette.cxx -I./fltk_contour/include

viewport.o: viewport.cpp objmodel.cpp 3dcontrols.o basic_ball.o FlGlArcballWindow.o Fl_Custom_Cursor.o fl_contour.o fl_gl_contour.o fl_palette.o
	g++ -g -c -O3 objmodel.cpp viewport.cpp `fltk-config --use-gl --cxxflags --ldflags` -L/opt/X11/lib -lglut -lGLU -I/opt/X11/include -I./multiarcball/src/FLui -I./multiarcball/src/math -I./multiarcball/src/flcursors/fltk1 -I./fltk_contour/include
