GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../i2c.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../ds18b20.o ../onewire.o ../crc8.o  ../adc.o ../temp.o ../node.o

CFLAGS = -DMY_ID=ID_diag -DONLCR=1 -DSECONDS_IRQ=1 -DECHO=1

include ../Makefile.avr
