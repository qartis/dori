GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../debug.o

CFLAGS = -DMY_ID=ID_modema -DUART_CUSTOM_INTERRUPT=1

include ../Makefile.avr
