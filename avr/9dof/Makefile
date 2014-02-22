GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 18432000L

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../i2c.o ../irq.o

CFLAGS = -DUART_BAUD=9600 -DUSER_IRQ=1 -DMY_ID=ID_9dof  -DUART_CUSTOM_INTERRUPT=1

include ../Makefile.avr
