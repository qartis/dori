all:
	g++ main.cpp radarwidget.cpp -o main `fltk-config --use-gl --cxxflags --ldflags`

test: all
	./main < data.txt
