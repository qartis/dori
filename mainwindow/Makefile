all: main

main: main.cpp widgetwindow.cpp widgetwindow.h mainwindow.h mainwindow.cpp queryinput.h queryinput.cpp table.h table.cpp ../radar/radarwindow.o ../3d/objmodel/viewport.o
	g++ -Wno-unused-result -Wformat=2 -Wall -Wextra -g main.cpp table.cpp mainwindow.cpp queryinput.cpp widgetwindow.cpp -L/opt/X11/lib -lglut -lGLU -I../3d/objmodel/multiarcball/src/math/ -I../3d/objmodel/fltk_contour/include ../radar/radarwindow.o ../3d/objmodel/objmodel.o ../3d/objmodel/3dcontrols.o ../3d/objmodel/basic_ball.o ../3d/objmodel/FlGlArcballWindow.o ../3d/objmodel/Fl_Custom_Cursor.o ../3d/objmodel/fl_contour.o ../3d/objmodel/fl_gl_contour.o ../3d/objmodel/fl_palette.o ../3d/objmodel/viewport.o  -lGL -lGLU `fltk-config --use-gl --cxxflags --ldflags` -lsqlite3 -o main -lXcursor

../radar/radarwindow.o: ../radar/radarwindow.cpp ../radar/radarwindow.h
	cd ../radar; make

../3d/objmodel/viewport.o: ../3d/objmodel/viewport.cpp ../3d/objmodel/viewport.h ../3d/objmodel/objmodel.cpp ../3d/objmodel/objmodel.h
	cd ../3d/objmodel; make

test: main
	./main
