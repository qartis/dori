GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L

LDFLAGS = -Wl,-section-start=.bootloader=0x7800,-section-start=.led=0x1000

include Makefile.avr

main.hex: main.elf
	avr-objcopy -j .text -j .led -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
