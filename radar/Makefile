all: main

main: radarwidget.cpp main.cpp tableview.o tableinput.o
	g++ -Wall -Wextra -g main.cpp radarwidget.cpp tableview.o tableinput.o -o main `fltk-config --use-gl --cxxflags --ldflags`

tableview.o: ../tableview/tableview.cpp
	g++ -Wall -Wextra -g -c ../tableview/tableview.cpp

tableinput.o: ../tableview/tableinput.cpp
	g++ -Wall -Wextra -g -c ../tableview/tableinput.cpp

test: main
	./main < data.txt
