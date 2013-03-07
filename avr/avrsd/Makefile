GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000

#LDFLAGS = -Wl,-section-start=.bootloader=0x3800 

CFLAGS = -DMY_ID=ID_LOGGER

include ../Makefile.avr

#main.hex: main.elf
#	avr-objcopy -j .text -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
