all: modem

modem: modem.c
	gcc -g -Wall -Wextra modem.c -o modem

test: modem
	rlwrap ./modem
