GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../debug.o

CFLAGS = -DUART_CUSTOM_INTERRUPT=1 -DMY_ID=ID_laser

include ../Makefile.avr
