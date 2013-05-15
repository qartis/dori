all: modem

modem: modem.c
	gcc -g -Wall -Wextra modem.c -o modem -lpthread

test: modem
	rlwrap ./modem | tee log.txt
