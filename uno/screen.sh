#!/bin/bash

# Function to cleanup screen session on exit
cleanup() {
    echo "Cleaning up screen session..."
    # Kill any remaining screen sessions for this device
    screen -S arduino -X quit 2>/dev/null || true
}

# Set trap to cleanup on script exit (including when Zed task is terminated)
trap cleanup EXIT INT TERM

# Start screen with a named session
TERM=xterm screen -S arduino `ls /dev/cu.usbmodem*` 9600,cs8,-cstopb,-parenb,-icrnl,-onlcr,-echo,-icanon
