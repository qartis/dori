GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L

PROG_DEVICE = /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A4007hkl-if00-port0

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../irq.o

CFLAGS = -DMY_ID=ID_modema

include ../Makefile.avr
