CC=/usr/bin/avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=atmega32
OBJ2HEX=/usr/bin/avr-objcopy
PROG=/usr/bin/avrdude
TARGET=clock
# disable JTAG
HFUSE=0x4f
LFUSE=0xe1

.PHONY: program
program: $(TARGET).hex
	$(PROG) -c usbasp -p ATmega32 -e
	$(PROG) -c usbasp -p ATmega32 -U flash:w:$(TARGET).hex -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

%.obj: %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

.PHONY: clean
clean:
	-rm -f *.hex *.obj *.o
