# Project:  tinyOLEDdemo
# Author:   Stefan Wagner
# Year:     2020
# URL:      https://easyeda.com/wagiminator
#           https://github.com/wagiminator
#
# Type "make help" in the command line.

# Microcontroller Options
DEVICE  = attiny10
CLOCK   = 8000000
PROGRMR = usbasp
FUSE    = 0xff

# Objects
OBJECTS = main.o

# Commands
CC       = avr-gcc
OBJCOPY  = avr-objcopy
OBJDUMP  = avr-objdump
AVRSIZE  = avr-size
AVRDUDE = avrdude -c $(PROGRMR) -p $(DEVICE)
COMPILE = $(CC) -Wall -Os -flto -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DDEBUG_LEVEL=0
CLEAN   = rm -f main.lst main.obj main.cof main.list main.map main.eep.hex *.o main.s

# Symbolic Targets
all:	help

help:
	@echo "Use the following commands:"
	@echo "make hex       compile and build main.hex for $(DEVICE)"
	@echo "make asm       compile and disassemble to main.asm for $(DEVICE)"
	@echo "make bin       compile and build main.bin for $(DEVICE)"
	@echo "make upload    compile and upload to $(DEVICE) using $(PROGRMR)"
	@echo "make fuses     burn fuses of $(DEVICE) using $(PROGRMR) programmer"
	@echo "make install   compile, upload and burn fuses for $(DEVICE)"
	@echo "make clean     remove all build files"

elf:	$(OBJECTS)
	@echo "Building main.elf ..."
	@$(COMPILE) -o main.elf $(OBJECTS)
	@echo "Removing temporary files ..."
	@$(CLEAN)

bin:	elf size
	@echo "Building main.bin ..."
	@$(OBJCOPY) -O binary -R .eeprom main.elf main.bin
	@echo "Removing temporary files ..."
	@$(CLEAN)

hex:	elf size
	@echo "Building main.hex ..."
	@$(OBJCOPY) -j .text -j .data -O ihex main.elf main.hex

asm:	elf size
	@echo "Disassembling to main.asm ..."
	@$(OBJDUMP) -d main.elf > main.asm

upload:	hex
	@echo "Uploading to $(DEVICE) ..."
	@$(AVRDUDE) -U flash:w:main.hex:i

fuses:
	@echo "Burning fuses of $(DEVICE) ..."
	@$(AVRDUDE) -U fuse:w:$(FUSE):m

install: upload fuses

flash: upload fuses

size:
	@echo "------------------"
	@echo "FLASH: $(shell $(AVRSIZE) -d main.elf | awk '/[0-9]/ {print $$1 + $$2}') bytes"
	@echo "SRAM:  $(shell $(AVRSIZE) -d main.elf | awk '/[0-9]/ {print $$2 + $$3}') bytes"
	@echo "------------------"

clean:
	@echo "Cleaning all up ..."
	@$(CLEAN)
	@rm -f main.bin main.hex main.asm main.elf

.c.o:
	@echo "Compiling $< for $(DEVICE) @ $(CLOCK)Hz ..."
	@$(COMPILE) -c $< -o $@

.S.o:
	@echo "Compiling $< for $(DEVICE) @ $(CLOCK)Hz ..."
	@$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	@echo "Compiling $< for $(DEVICE) @ $(CLOCK)Hz ..."
	@$(COMPILE) -S $< -o $@
