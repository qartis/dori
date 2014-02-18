GCC_PART = atmega328p
DUDE_PART = m328p
F_CPU = 18432000L

OBJS = ../uart.o ../mcp2515.o ../time.o ../irq.o ../debug.o ../spi.o #../sd.o ../fat.o

CFLAGS = -DMY_ID=ID_logger -DCAN_TOPHALF=1 -DUSER_IRQ=1 -DDEBUG=1 -DUART_CUSTOM_INTERRUPT=1 -DUART_BAUD=38400

include ../Makefile.avr

#main.hex: main.elf
#	avr-objcopy -j .text -j .data -j .bootloader -O ihex main.elf main.hex
#	avr-size main.elf
#avr-size -C --mcu=$(GCC_PART) main.elf
