MCU ?= atmega128

ARDUINO_FILES ?= /home/jon/Downloads/arduino-1.8.16/hardware
ARDUINO_SRCS = $(ARDUINO_FILES)/tools/avr/avr/include

ARDUINO_PORT ?= /dev/ttyACM0


GPP_FLAGS := -c -g -O2 -Wall -fno-exceptions -ffunction-sections -fdata-sections -mmcu=$(MCU) -DF_CPU=8000000L -DARDUINO=101 -I $(ARDUINO_SRCS)


sounds.hex: sounds.elf
	avr-objcopy -O ihex -R .eeprom sounds.elf sounds.hex

sounds.elf: sounds_main.o sound0.o sound1.o sound2.o sound3.o
	$(ARDUINO_FILES)/tools/avr/bin/avr-g++ -O2 -mmcu=$(MCU) -o sounds.elf sounds_main.o sound0.o sound1.o sound2.o sound3.o -lm

sound0.o: sound0.c
	$(ARDUINO_FILES)/tools/avr/bin/avr-g++ $(GPP_FLAGS) sound0.c

sound1.o: sound1.c
	$(ARDUINO_FILES)/tools/avr/bin/avr-g++ $(GPP_FLAGS) sound1.c

sound2.o: sound2.c
	$(ARDUINO_FILES)/tools/avr/bin/avr-g++ $(GPP_FLAGS) sound2.c

sound3.o: sound3.c
	$(ARDUINO_FILES)/tools/avr/bin/avr-g++ $(GPP_FLAGS) sound3.c

sounds_main.o: sounds_main.c
	$(ARDUINO_FILES)/tools/avr/bin/avr-gcc $(GPP_FLAGS) sounds_main.c

upload: sounds.hex
	$(ARDUINO_FILES)/tools/avr/bin/avrdude -C$(ARDUINO_FILES)/tools/avr/etc/avrdude.conf -v -v -v -p$(MCU) -cstk500v1 -P$(ARDUINO_PORT) -b19200 -Uflash:w:sounds.hex:i

fuses:
	$(ARDUINO_FILES)/tools/avr/bin/avrdude -C$(ARDUINO_FILES)/tools/avr/etc/avrdude.conf -v -v -v -p$(MCU) -cstk500v1 -P$(ARDUINO_PORT) -b19200 -U lfuse:w:0xe4:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m

clean:
	rm -f *.o *.hex *.elf

