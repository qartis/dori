GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000L

PROG_DEVICE = /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A4007hkl-if00-port0

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o

CFLAGS = -DMY_ID=ID_any

include ../Makefile.avr
