all: main

main: main.cpp tableview.cpp tableinput.cpp table.cpp
	g++ -Wall -Wextra -g main.cpp table.cpp tableview.cpp tableinput.cpp -o main `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

test: main
	./main
