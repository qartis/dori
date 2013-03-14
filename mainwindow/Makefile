all: main

main: main.cpp widgetwindow.cpp widgetwindow.h mainwindow.h mainwindow.cpp queryinput.h queryinput.cpp table.h table.cpp ../radar/radarwindow.o ../viewport/viewport.o ../siteeditor/siteeditor.o ../siteeditor/toolbar.o ../siteeditor/lineobject.o ../siteeditor/rectobject.o ../siteeditor/circleobject.o
	g++ -Wno-unused-parameter -Wno-unused-result -Wformat=2 -Wall -Wextra -g main.cpp table.cpp mainwindow.cpp queryinput.cpp widgetwindow.cpp ../siteeditor/lineobject.o  ../siteeditor/circleobject.o ../siteeditor/rectobject.o -L/opt/X11/lib -L/usr/share/lib -lglut -lGLU -I../multiarcball/src/math/ -I../viewport -I../multiarcball/src/FLui/ -I../radar/ -I../ -I../siteeditor/ -I../multiarcball/src/flcursors/fltk1/ -I../mathgl/include ../radar/radarwindow.o ../viewport/objmodel.o ../viewport/3dcontrols.o ../viewport/basic_ball.o ../viewport/FlGlArcballWindow.o ../viewport/Fl_Custom_Cursor.o ../viewport/viewport.o ../siteeditor/siteeditor.o ../siteeditor/toolbar.o -lmgl -lmgl-fltk -lGL -lGLU `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -o main -lXcursor

../radar/radarwindow.o: ../radar/radarwindow.cpp ../radar/radarwindow.h
	cd ../radar; make

../viewport/viewport.o: ../viewport/viewport.cpp ../viewport/viewport.h ../viewport/objmodel.cpp ../viewport/objmodel.h ../siteeditor/siteeditor.o
	cd ../viewport; make

../siteeditor/toolbar.o: ../siteeditor/siteeditor.o

../siteeditor/siteeditor.o: ../siteeditor/siteeditor.cpp ../siteeditor/siteeditor.h
	cd ../siteeditor; make

test: main
	./main

clean:
	rm main
