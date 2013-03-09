all: main

main: siteeditor.o toolbar.o

siteeditor.o: siteeditor.cpp siteeditor.h
	g++ -Wall -Wextra -g siteeditor.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

toolbar.o: toolbar.cpp toolbar.h geometry.h line.h rect.h circle.h
	g++ -Wall -Wextra -g toolbar.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

clean:
	rm *.o
