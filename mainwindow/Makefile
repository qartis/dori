all: main

main: main.cpp mainwindow.h mainwindow.cpp queryinput.h queryinput.cpp table.h table.cpp ../radar/radarwindow.o
	g++ -Wformat=2 -Wall -Wextra -g main.cpp table.cpp mainwindow.cpp queryinput.cpp ../radar/radarwindow.o -lGL -lGLU `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -o main

radarwindow.o:
	make ../radar/Makefile

test: main
	./main
