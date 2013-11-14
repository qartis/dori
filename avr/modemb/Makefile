GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../debug.o ../irq.o

CFLAGS = -DUSER_IRQ=1 -DUART_CUSTOM_INTERRUPT=1 -DMY_ID=ID_modemb -DDEBUG=1


include ../Makefile.avr
