GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../spi.o ../i2c.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../ds18b20.o ../onewire.o ../crc8.o ../temp.o ../adc.o ../debug.o ../node.o

CFLAGS = -DMY_ID=ID_enviro -DONLCR=1 -DDEBUG=1

include ../Makefile.avr
