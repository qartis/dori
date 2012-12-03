all:
	g++ main.cpp objmodel.cpp viewport.cpp -o main `fltk-config --use-gl --cxxflags --ldflags` -L/opt/X11/lib -lGLU -I/opt/X11/include

test:
	./main robot.obj
