all: main

main: radarwindow.cpp spawnablewindow.o
	g++ -Wall -Wextra -g radarwindow.cpp -c `fltk-config --use-gl --cxxflags --ldflags`

spawnablewindow.o:
	g++ -Wall -Wextra -g -c ../spawnablewindow.h
