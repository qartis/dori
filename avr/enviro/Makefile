GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000

OBJS = ../spi.o ../i2c.o ../uart.o ../mcp2515.o ../time.o ../irq.o ../ds18b20.o ../onewire.o ../crc8.o ../temp.o ../adc.o

CFLAGS = -DMY_ID=ID_enviro -DONLCR=1

include ../Makefile.avr
