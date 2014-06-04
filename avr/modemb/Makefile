GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../debug.o ../node.o

CFLAGS = -DUSER1_IRQ=1 -DUART_CUSTOM_INTERRUPT=1 -DMY_ID=ID_modemb -DDEBUG=1 -DUART_BAUD=115200 -DDEBUG_FAST=1 -DDEFAULT_PERIOD=5


include ../Makefile.avr
