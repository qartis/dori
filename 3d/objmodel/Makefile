all:
	g++ main.cpp objmodel.cpp viewport.cpp arcball.cpp -o main `fltk-config --use-gl --cxxflags --ldflags` -L/opt/X11/lib -lglut -lGLU -I/opt/X11/include

test:
	./main robot.obj box.obj
