GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../debug.o ../irq.o

CFLAGS = -DMY_ID=ID_modema -DUART_CUSTOM_INTERRUPT=1 -DDEBUG=1 -DUART_BAUD=9600 -DUSER_IRQ=1


include ../Makefile.avr
