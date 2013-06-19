GCC_PART = atmega88p
DUDE_PART = m88p
F_CPU = 8000000L

#LDFLAGS = -Wl,-section-start=.bootloader=0x3800 

OBJS = ../spi.o ../uart.o ../mcp2515.o ../time.o ../fat.o ../sd.o

CFLAGS = -DMY_ID=ID_sd

include ../Makefile.avr

#main.hex: main.elf
#	avr-objcopy -j .text -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
