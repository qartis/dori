all:
	g++ -c ./multiarcball/src/math/*.cpp
	g++ -c ./multiarcball/src/FLui/*.cpp -I./multiarcball/src/math/
	g++ -c ./multiarcball/src/flcursors/fltk1/*.cxx
	g++ main.cpp objmodel.cpp viewport.cpp arcball.cpp basic_ball.o 3dcontrols.o FlGlArcballWindow.o Fl_Custom_Cursor.o -lXcursor -o main `fltk-config --use-gl --cxxflags --ldflags` -L/opt/X11/lib -lglut -lGLU -I/opt/X11/include -I./multiarcball/src/FLui -I./multiarcball/src/math -I./multiarcball/src/flcursors/fltk1

test:
	./main robot.obj box.obj
