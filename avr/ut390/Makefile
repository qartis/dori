GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../debug.o

CFLAGS = -DUART_CUSTOM_INTERRUPT=1 -DMY_ID=ID_lidar

include ../Makefile.avr
