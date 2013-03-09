all: server mainwindow

server: server.c
	gcc -Wall -Wextra -g server.c -o server -lsqlite3

clean:
	rm server

