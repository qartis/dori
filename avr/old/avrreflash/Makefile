GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 8000000L
FLASH_SECTION = .flashed
FLASH_SECTION_ADDR = 0x2000

LDFLAGS = -Wl,-section-start=.bootloader=0x7800,-section-start=$(FLASH_SECTION)=$(FLASH_SECTION_ADDR)

include Makefile.avr

main.hex: main.elf
	avr-objcopy -j .text -j .flashed -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
