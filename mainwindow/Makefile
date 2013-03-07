all: main

main: main.cpp widgetwindow.cpp widgetwindow.h mainwindow.h mainwindow.cpp queryinput.h queryinput.cpp table.h table.cpp ../radar/radarwindow.o ../viewport/viewport.o ../siteeditor/siteeditor.o
	g++ -Wno-unused-result -Wformat=2 -Wall -Wextra -g main.cpp table.cpp mainwindow.cpp queryinput.cpp widgetwindow.cpp -L/opt/X11/lib -lglut -lGLU -I../multiarcball/src/math/ -I../viewport -I../multiarcball/src/FLui/ -I../radar/ -I../ -I../siteeditor/ -I../multiarcball/src/flcursors/fltk1/ ../radar/radarwindow.o ../viewport/objmodel.o ../viewport/3dcontrols.o ../viewport/basic_ball.o ../viewport/FlGlArcballWindow.o ../viewport/Fl_Custom_Cursor.o ../viewport/viewport.o ../siteeditor/siteeditor.o -lGL -lGLU `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -o main -lXcursor

../radar/radarwindow.o: ../radar/radarwindow.cpp ../radar/radarwindow.h
	cd ../radar; make

../viewport/viewport.o: ../viewport/viewport.cpp ../viewport/viewport.h ../viewport/objmodel.cpp ../viewport/objmodel.h
	cd ../viewport; make

../siteeditor/siteeditor.o: ../siteeditor/siteeditor.cpp ../siteeditor/siteeditor.h
	cd ../siteeditor; make

test: main
	./main
