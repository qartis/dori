all:
	g++ main.cpp viewport.cpp -o main `fltk-config --use-gl --cxxflags --ldflags`

test:
	./main < nunchuck2.txt
