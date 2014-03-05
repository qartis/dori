GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o

CFLAGS = -DMY_ID=ID_drive -DONLCR=1 -DECHO=1

include ../Makefile.avr
