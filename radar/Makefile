all: main

main: main.cpp radarwidget.cpp
	g++ -g main.cpp radarwidget.cpp -o main `fltk-config --use-gl --cxxflags --ldflags`

test: main
	./main < data.txt
