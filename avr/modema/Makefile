GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../debug.o ../irq.o

CFLAGS = -DMY_ID=ID_modema -DUART_CUSTOM_INTERRUPT=1 -DDEBUG=1 -DUART_BAUD=115200 -DUSER_IRQ=1 -DDEFAULT_PERIOD=30


include ../Makefile.avr
