GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L

PROG_DEVICE = /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A4007hkl-if00-port0

#LDFLAGS = -Wl,-section-start=.bootloader=0x3800 

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../fat.o ../sd.o

CFLAGS = -DMY_ID=ID_sd
#CFLAGS += -D"printf_P(...)"=";" -D"puts_P(...)"=";"
#CFLAGS += -D"printf_P(...)"=";"

include ../Makefile.avr

#main.hex: main.elf
#	avr-objcopy -j .text -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
