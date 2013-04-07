all: server mainwindow

server: server.c shell_monitor
	gcc -Wall -Wextra -g3 server.c -o server -lsqlite3
	sudo chown root:root server
	sudo chmod +s server


shell_monitor:
	mkfifo shell_monitor

clean:
	rm server shell_monitor

