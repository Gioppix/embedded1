# Parachute 0.5

Demo Video: `./pitch.mp4` (video file not accessible via markdown link)

### Goals

- Bare metal programming, no libraries
    - Only `math.h` (trigonometric functions) and `stdint.h` (cross-platform types) used
- No IDEs that take care of the build/flash process
- Performance (60 fps target)

### Non-Goals

- Complexity

## Requirements to run

This project was only tested on MacOS, but supports Linux

### Backend

- Hardware
    - Arduino UNO board
    - Button
    - Potentiometer
    - 20x4 Lcd 2004 + appropriate resistors (Optional, debug)
    - RGB LED + appropriate resistors (Optional, debug)
- Software
    - Avr tooling (see `flash.sh` for details)

### Frontend

- Software
    - Chrome (or any other browser with `WebSerial` support)
    - [Bun](https://bun.sh/)

## Project layout

```
├── readme.md
├── screen.sh                # Convenience script to connect to USART
├── generate-types.sh        # Script that generates shared Ts and C code
├── flash.sh                 # All-in-one utility to compile and flash to Arduino
├── frontend                 # Frontend application
└── src
     ├── analog               # ADC-related
     ├── game                 # Main game logic/rendering
     ├── lcd2004              # LCD 2004
     ├── serial               # USART
     ├── timers               # Timers and utilities for time
     ├── two_wires            # Two Wires Interface
     ├── utils                # General utilities
     ├── gen_queue.h          # Macro to generate a circual buffer
     ├── generated.h          # Automatically generated file
     ├── main.c
     └── ports.h              # Commonly used ports
```

## Compiling/burning

To compile and burn the source into a connected Arduino, use the provided script:

```
./flash.sh
```

You can inspect the disassembly in build/firmware_disasm.s

To start the frontend, use

```
cd frontend && bun dev
```

## Developing

During development, use the following command:

```
./generate-types.sh && sleep 1 && ./flash.sh
```

This ensures shared enums and constants stay synchronized between the frontend and backend. Additionally, it automatically disconnects any active frontend connections, freeing the serial port for the flashing process.
The disconnection works thanks to `Vite`'s live reload: editing `generated.ts` will reload the page, thus closing the connection.

## Next steps

- Really understand [what's going on](https://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_libm)
- a
