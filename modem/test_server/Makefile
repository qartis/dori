all: test_server

test_server: test_server.c
	gcc -g -Wall -Wextra test_server.c -o test_server -lpthread

test: test_server
	rlwrap ./test_server
