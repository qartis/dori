all: main

main: siteeditor.o

siteeditor.o: siteeditor.cpp siteeditor.h
	g++ -Wall -Wextra -g siteeditor.cpp -c `fltk-config --use-gl --cxxflags --ldflags`
