all: main

main: siteeditor.o toolbar.o

toolbar.o: toolbar.cpp toolbar.h siteeditor.o
	g++ -Wall -Wextra -g toolbar.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

siteeditor.o: siteeditor.cpp siteeditor.h siteobject.h lineobject.o rectobject.o circleobject.o
	g++ -Wno-unused-parameter -Wall -Wextra -g siteeditor.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

lineobject.o:
	g++ -Wno-unused-parameter -Wall -Wextra -g lineobject.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

rectobject.o:
	g++ -Wno-unused-parameter -Wall -Wextra -g rectobject.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

circleobject.o:
	g++ -Wno-unused-parameter -Wall -Wextra -g circleobject.cpp -c `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3

clean:
	rm *.o
