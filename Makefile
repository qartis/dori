all: server mainwindow

server: server.c
	gcc -Wall -Wextra -g server.c -o server -lsqlite3

mainwindow: mainwindow/main.cpp mainwindow/widgetwindow.cpp mainwindow/widgetwindow.h mainwindow/mainwindow.h mainwindow/mainwindow.cpp mainwindow/queryinput.h mainwindow/queryinput.cpp mainwindow/table.h mainwindow/table.cpp radar/radarwindow.o viewport/viewport.o siteeditor/siteeditor.o
	cd mainwindow; make
