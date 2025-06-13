#!/bin/bash

# Explanation:
# https://www.nongnu.org/avr-libc/user-manual/group__demo__project.html

# Exit on any error
set -e

# Configuration
MCU="atmega328p"
F_CPU="16000000UL"
PROGRAMMER="arduino"
BAUD="115200"
TARGET="firmware"
SRC_DIR="src"
BUILD_DIR="build"
DISASM_FILE="${BUILD_DIR}/${TARGET}_disasm.s"

rm -rf $BUILD_DIR

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

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

# Check if src directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "❌ Source directory '$SRC_DIR' not found"
    exit 1
fi

# Find all .c files in src directory
C_FILES=$(find $SRC_DIR -name "*.c" -type f)

if [ -z "$C_FILES" ]; then
    echo "❌ No .c files found in $SRC_DIR directory"
    exit 1
fi

echo "🔧 Compiling and linking all source files..."

# Compile and link all .c files in one command
avr-gcc -mmcu=$MCU -Wall -O3 -DF_CPU=$F_CPU -I$SRC_DIR -o $BUILD_DIR/$TARGET.elf $C_FILES
if [ $? -ne 0 ]; then
    echo "❌ Compilation and linking failed"
    exit 1
fi

echo "📝 Creating hex file..."

# Create the hex file
avr-objcopy -O ihex -R .eeprom $BUILD_DIR/$TARGET.elf $BUILD_DIR/$TARGET.hex
if [ $? -ne 0 ]; then
    echo "❌ Hex file creation failed"
    exit 1
fi

echo "📤 Flashing to Arduino on $PORT..."

# Flash the hex file to the Arduino
avrdude -p $MCU -c $PROGRAMMER -P $PORT -b $BAUD -D -U flash:w:$BUILD_DIR/$TARGET.hex:i
if [ $? -ne 0 ]; then
    echo "❌ Flashing failed"
    exit 1
fi

echo "✅ Done! Your program has been successfully flashed to the Arduino."

echo "🔎 Generating disassembled assembly (final code for Arduino)..."

# Disassemble the ELF file to reveal the final assembly instructions
avr-objdump -d $BUILD_DIR/$TARGET.elf > $DISASM_FILE
if [ $? -ne 0 ]; then
    echo "❌ Disassembly failed"
    exit 1
fi

avr-size -C --mcu=atmega328p build/firmware.elf

echo "📝 Disassembled assembly saved to $DISASM_FILE"
echo "📁 All build artifacts are in the '$BUILD_DIR' directory"
