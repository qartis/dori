GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../debug.o ../irq.o

CFLAGS = -DMY_ID=ID_modemb -DDEBUG=1 -DUART_BAUD=9600


include ../Makefile.avr
