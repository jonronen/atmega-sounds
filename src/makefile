MCU ?= atmega128

ARDUINO_FILES ?= /home/jon/Downloads/arduino-1.8.5
ARDUINO_SRCS = $(ARDUINO_FILES)/hardware/arduino/avr/cores/arduino

ARDUINO_PORT ?= /dev/ttyACM0


GPP_FLAGS := -c -g -O2 -Wall -fno-exceptions -ffunction-sections -fdata-sections -mmcu=$(MCU) -DF_CPU=8000000L -DARDUINO=101 -I$(ARDUINO_SRCS) -I$(ARDUINO_FILES)/hardware/arduino/avr/variants/standard


sounds.hex: sounds.elf
	avr-objcopy -O ihex -R .eeprom sounds.elf sounds.hex

sounds.elf: sounds_main.o sound0.o sound1.o sound2.o sound3.o
	avr-g++ -O2 -mmcu=$(MCU) -o sounds.elf sounds_main.o sound0.o sound1.o sound2.o sound3.o -lm

sound0.o: sound0.c
	avr-g++ $(GPP_FLAGS) sound0.c

sound1.o: sound1.c
	avr-g++ $(GPP_FLAGS) sound1.c

sound2.o: sound2.c
	avr-g++ $(GPP_FLAGS) sound2.c

sound3.o: sound3.c
	avr-g++ $(GPP_FLAGS) sound3.c

sounds_main.o: sounds_main.c
	avr-gcc $(GPP_FLAGS) sounds_main.c

upload: sounds.hex
	avrdude -C$(ARDUINO_FILES)/hardware/tools/avr/etc/avrdude.conf -v -p$(MCU) -cstk500v1 -P$(ARDUINO_PORT) -b19200 -Uflash:w:sounds.hex:i

clean:
	rm -f *.o *.hex *.elf

