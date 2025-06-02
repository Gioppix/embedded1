#!/bin/bash

# Exit on any error
set -e

# Configuration
MCU="atmega328p"
F_CPU="16000000UL"
PROGRAMMER="arduino"
BAUD="115200"
TARGET="blink"
SRC_FILE="main.c"
DISASM_FILE="${TARGET}_disasm.s"  # file to hold disassembled code

# Detect Arduino port automatically (common patterns on macOS)
PORT=$(ls /dev/cu.usbmodem* 2>/dev/null || ls /dev/cu.usbserial* 2>/dev/null || echo "")

if [ -z "$PORT" ]; then
    echo "Error: Arduino not found. Please connect your Arduino."
    exit 1
fi

echo "📌 Using Arduino on port: $PORT"

# Check if required tools are installed
command -v avr-gcc >/dev/null 2>&1 || { echo "❌ avr-gcc not found. Install with: brew install avr-gcc"; exit 1; }
command -v avr-objcopy >/dev/null 2>&1 || { echo "❌ avr-objcopy not found. Install with: brew install avr-gcc"; exit 1; }
command -v avrdude >/dev/null 2>&1 || { echo "❌ avrdude not found. Install with: brew install avrdude"; exit 1; }
command -v avr-objdump >/dev/null 2>&1 || { echo "❌ avr-objdump not found. Install with: brew install avr-gcc"; exit 1; }

echo "🔧 Compiling $SRC_FILE and emitting assembly source..."

# Compile the source file and emit the assembly file using -save-temps.
# This preserves the intermediate assembly output (main.s).
avr-gcc -mmcu=$MCU -Wall -O0 -DF_CPU=$F_CPU -save-temps -c $SRC_FILE -o ${SRC_FILE%.c}.o
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed"
    exit 1
fi

echo "🔗 Linking..."

# Link the object file
avr-gcc -mmcu=$MCU -o $TARGET.elf ${SRC_FILE%.c}.o
if [ $? -ne 0 ]; then
    echo "❌ Linking failed"
    exit 1
fi

echo "📝 Creating hex file..."

# Create the hex file
avr-objcopy -O ihex -R .eeprom $TARGET.elf $TARGET.hex
if [ $? -ne 0 ]; then
    echo "❌ Hex file creation failed"
    exit 1
fi

echo "📤 Flashing to Arduino on $PORT..."

# Flash the hex file to the Arduino
avrdude -p $MCU -c $PROGRAMMER -P $PORT -b $BAUD -D -U flash:w:$TARGET.hex:i
if [ $? -ne 0 ]; then
    echo "❌ Flashing failed"
    exit 1
fi

echo "✅ Done! Your program has been successfully flashed to the Arduino."

echo "🔎 Generating disassembled assembly (final code for Arduino)..."

# Disassemble the ELF file to reveal the final assembly instructions.
avr-objdump -d $TARGET.elf > $DISASM_FILE
if [ $? -ne 0 ]; then
    echo "❌ Disassembly failed"
    exit 1
fi

echo "📝 Disassembled assembly saved to $DISASM_FILE"

# Clean up intermediate files (keeping the disassembled file and the generated assembly from -save-temps)
echo "🧹 Cleaning up intermediate object and ELF files..."
rm -f *.o *.elf *.hex
