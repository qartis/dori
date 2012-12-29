all: main

main: main.cpp tableview.cpp tableinput.cpp
	g++ -g main.cpp tableview.cpp tableinput.cpp -o main `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

test: main
	./main
