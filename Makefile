FREQ=16000000
MCU=atmega328p
PROGRAMMER=arduino
PORT=/dev/ttyACM0

CC=avr-gcc
LD=avr-gcc
OBJCOPY=avr-objcopy
AR=avr-ar
RM=rm -f
AVRDUDE=avrdude

CFLAGS=-mmcu=$(MCU) -DF_CPU=$(FREQ)UL -g -std=c11 -Wall -Wextra -pedantic \
       -fshort-enums -fpack-struct -ffunction-sections -fdata-sections -Os \
       -Isrc -flto
LDFLAGS=-mmcu=$(MCU) -Wl,--gc-sections -Os -flto

TARGET=lasertag
TARGET_HEX=$(TARGET).hex

SOURCES=$(shell find src -name "*.c")
OBJECTS=$(addsuffix .o, $(basename $(SOURCES)))
DEPENDENCIES=$(shell find src -name "*.d")

.PHONY: all clean upload

all: $(TARGET_HEX)

upload: all
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -U flash:w:$(TARGET_HEX):i

clean:
	$(RM) $(TARGET) $(TARGET_HEX) $(OBJECTS) $(DEPENDENCIES)

$(TARGET_HEX): $(TARGET)
	$(OBJCOPY) -O ihex $< $@

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -MMD -MP -MQ $@ -MF $(addsuffix .d, $(basename $@)) -c -o $@ $<

include $(DEPENDENCIES)

