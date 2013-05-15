all: main

main: radarwindow.cpp radarwindow.h
	g++ -Wall -Wextra -g radarwindow.cpp -c `fltk-config --use-gl --cxxflags --ldflags`

clean: radarwindow.o
