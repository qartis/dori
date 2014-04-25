GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../i2c.o ../irq.o ../debug.o

CFLAGS = -DUART_BAUD=9600 -DUSER1_IRQ=1 -DMY_ID=ID_9dof  -DUART_CUSTOM_INTERRUPT=1 -DDEBUG=1

include ../Makefile.avr
