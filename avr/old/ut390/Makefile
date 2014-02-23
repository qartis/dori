GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../debug.o ../adc.o

CFLAGS = -DUART_CUSTOM_INTERRUPT=1 -DMY_ID=ID_imaging -DUART_BAUD=115200 -DDEBUG=1

include ../Makefile.avr
