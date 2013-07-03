all: modem

modem: main.c sim900.c
	gcc -g -Wall -Wextra main.c sim900.c -o modem -lpthread

test: modem
	rlwrap ./modem | tee log.txt
clean: 
	rm ./modem
