GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000

OBJS = ../spi.o ../i2c.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../ds18b20.o ../onewire.o ../crc8.o 

CFLAGS = -DMY_ID=ID_diag -DONLCR=1

include ../Makefile.avr
