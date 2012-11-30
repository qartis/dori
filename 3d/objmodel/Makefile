all:
	g++ main.cpp objmodel.cpp viewport.cpp -o main `fltk-config --use-gl --cxxflags --ldflags` -lGLU

test:
	./main robot.obj
