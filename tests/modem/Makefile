all: modem

modem: modem.c sim900.c
	gcc -g -Wall -Wextra modem.c sim900.c -o modem -lpthread

test: modem
	rlwrap ./modem | tee log.txt
clean: 
	rm ./modem
