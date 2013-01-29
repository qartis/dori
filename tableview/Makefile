all: main

main: main.cpp tableview.cpp tableinput.cpp table.cpp ../radar/radarwindow.o
	g++ -Wformat=2 -Wall -Wextra -g main.cpp table.cpp tableview.cpp tableinput.cpp ../radar/radarwindow.o -lGL -lGLU `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -o main 

radarwindow.o:
	make ../radar/Makefile

test: main
	./main
