# Morse Code Interaction System on ATmega324A

An embedded C project for entering, decoding, and playing Morse code on an ATmega324A.
The system combines push buttons, a serial terminal, a 16 x 8 LED matrix, a two-digit seven-segment display, a joystick, and a buzzer.
It was developed from the CSSE2010/CSSE7201 AVR project base code for Semester One, 2026.

## Contents

- [Features](#features)
- [Hardware and wiring](#hardware-and-wiring)
- [Build and upload](#build-and-upload)
- [Using the system](#using-the-system)
- [Software structure](#software-structure)
- [EEPROM history](#eeprom-history)
- [Manual verification](#manual-verification)
- [Known limitations](#known-limitations)
- [Credits](#credits)

## Features

| Area | Current implementation |
| --- | --- |
| Button input | Separate dot, dash, and submit buttons, with rising-edge detection and a 20 ms debounce filter |
| Timed input | Single-button Morse entry selected by switch S0 |
| LED matrix | Red character previews, green button submissions, yellow serial submissions, and left-shift animation |
| IO board LEDs | An eight-beat history of marks and gaps, with queued animation |
| Seven-segment display | Completed-character count in hexadecimal and current mark count |
| Serial terminal | Uppercase text, live previews, and matching red, green, and yellow colours |
| Sound | Serial-input Morse playback with separate dot and dash tones |
| Fonts | Three-column and five-column glyphs selected by switch S1 |
| Joystick | Column-by-column history scrolling and 15 brightness levels |
| Persistence | Character and colour history stored in a 64-slot EEPROM ring |

The tables below describe the current source code.
Hardware operation and full assignment compliance have not been verified by this documentation update.
See [Known limitations](#known-limitations) for issues found during source review.

## Hardware and wiring

### Required hardware

- ATmega324A board running at an actual CPU clock of 8 MHz
- Course IO board with buttons, switches, and eight LEDs
- EECS-compatible 16 x 8 LED matrix using the course SPI command protocol
- Two-digit seven-segment display
- Analogue joystick with X and Y outputs
- Buzzer suitable for the board's PWM output
- Programmer supporting the configured AVRDUDE `stk500v2` protocol
- Serial connection between the MCU UART and a computer

The matrix driver uses the EECS matrix protocol.
A generic SPI display needs its own matching driver and cannot be assumed to work with this firmware.
Use the board's hardware documentation for power, ground, connectors, and physical package pin numbers.

### MCU pin assignments

| Peripheral signal | MCU pin | Firmware behaviour |
| --- | --- | --- |
| B0 | PB0 | Dot input or timed input; active high |
| B1 | PB1 | Dash input; active high |
| B2 | PB2 | Submit input; active high |
| Matrix SS | PB4 | SPI slave select |
| Matrix MOSI | PB5 | SPI data output |
| Matrix SCK | PB7 | SPI clock output |
| Joystick X | PA0 / ADC0 | Horizontal position |
| Joystick Y | PA1 / ADC1 | Vertical position |
| IO LEDs L0-L5 | PA2-PA7 | Low six bits of LED history |
| IO LEDs L6-L7 | PD6-PD7 | High two bits of LED history |
| Seven-segment A-G, DP | PC0-PC7 | Segment outputs, in that order |
| Seven-segment digit select | PD2 | High selects the left digit; low selects the right digit |
| S0 | PD3 | Low: asynchronous input; high: synchronous input |
| S1 | PD4 | Low: three-column font; high: five-column font |
| Buzzer | PD5 / OC1A | Timer1 PWM output |
| UART RX | PD0 / RXD0 | Receive from the computer's TX signal |
| UART TX | PD1 / TXD0 | Transmit to the computer's RX signal |

PB6 is the hardware SPI MISO pin; the matrix write path does not explicitly configure it.
PB3 is not used by the application.
The button and switch inputs have their internal pull-ups disabled, so the connected hardware must provide defined low and high levels.

### Timer allocation

| Timer | Configuration | Purpose |
| --- | --- | --- |
| Timer0 | CTC, divide-by-64, `OCR0A = 124` | 1 ms timebase and queued 100 ms animation ticks |
| Timer1 | Fast PWM mode 14, no prescaler | Buzzer frequency and duty cycle |
| Timer2 | CTC, divide-by-64, `OCR2A = 124` | Seven-segment multiplexing every 1 ms |

These timings depend on an actual 8 MHz CPU clock.
Setting `board_build.f_cpu` describes the clock to the build system; it does not program clock fuses.

## Build and upload

### Development environment

Install PlatformIO Core, or use VS Code with the PlatformIO IDE extension.
The project uses AVR C and AVR-LibC, without an Arduino framework.
Run the commands below from the directory containing `platformio.ini`.

The checked-in configuration selects:

| Setting | Value |
| --- | --- |
| Environment | `ATmega324A` |
| Platform | `atmelavr` |
| Board | `ATmega324A` |
| Declared CPU frequency | `8000000L` |
| Optimisation | `-Og`; `-Os` and `-flto` removed |
| Upload port | `COM7` |
| Upload tool | Custom AVRDUDE command |
| Programmer protocol | `stk500v2` |

### Build the firmware

```sh
pio run -e ATmega324A
```

A successful build produces `firmware.elf` and `firmware.hex` under `.pio/build/ATmega324A/`.
An existing output file does not prove that the current source builds successfully.
For compiler details, use:

```sh
pio run -e ATmega324A -v
```

### Upload to the board

List the connected serial devices:

```sh
pio device list
```

Update `upload_port` in `platformio.ini` to match the programmer, or override it for one upload:

```sh
pio run -e ATmega324A -t upload --upload-port COM7
```

The custom command writes flash with AVRDUDE using `stk500v2`.
It does not explicitly write EEPROM or clock fuses.
Select a programmer and port that match your hardware, and close any terminal using that port before uploading.

### Open the serial terminal

Use an ANSI/VT100-capable terminal with these settings:

| Setting | Value |
| --- | --- |
| Baud rate | 19200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Local echo | Off |
| Terminal size | 80 columns x 24 rows |

The programmer port and UART terminal port may be different.
For a basic connection through PlatformIO, replace `COM8` with the UART port:

```sh
pio device monitor --port COM8 --baud 19200
```

Use a terminal that renders cursor positioning and colour escape sequences correctly for the full interface.
Send individual characters as they are typed rather than waiting for an entire line.

Command references: [PlatformIO build and upload](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html) and [serial monitor](https://docs.platformio.org/en/latest/core/userguide/device/cmd_monitor.html).

## Using the system

### Start-up

The matrix and terminal show a splash screen.
Press B0, B1, or B2 to enter the emulator.
That initial press is consumed and is not reused as a Morse input.
Saved matrix history is restored from EEPROM when the emulator starts; the terminal and session counter start fresh.

### Asynchronous input: S0 low

| Control | Action |
| --- | --- |
| B0 | Add a dot |
| B1 | Add a dash |
| B2 after one or more marks | Submit the current character |
| B2 again after a submitted character | Add one word space |
| Further consecutive B2 presses | Ignore the input |

Pressing B2 before any character has been entered is also ignored by the current implementation.
Buttons act on a rising edge, so holding a button does not repeatedly add its input.
Another button can still be pressed while the first is held.

For example, enter `C` with B1, B0, B1, B0, then B2:

```text
Input:    -    .    -    .    submit
Preview:  T    N    K    C
Result:                      C
```

The incomplete character appears in red.
Submitting it changes it to green and starts its left-shift animation.
An unrecognised Morse sequence displays `?`.
The IO LEDs use one beat for a dot, three for a dash, one low beat between marks, three between characters, and five between words.

### Synchronous input: S0 high

Only B0 controls Morse entry in this mode.

| Event | Current code threshold |
| --- | --- |
| Release B0 after a short press | Less than 200 ms: dot |
| Release B0 after a long press | At least 200 ms: dash |
| Keep B0 released | At least 1000 ms: submit the character |
| Continue leaving B0 released | At least 2000 ms: submit one word space |
| Leave B0 released longer | No further submission |

B1 and B2 have no effect in synchronous mode.
Changing S0 discards the unfinished character and resets press/release timing while keeping completed matrix history.

### Serial input and sound

Type `A-Z`, `a-z`, or `0-9` in the serial terminal.
Letters are converted to uppercase, echoed in yellow, and added to the matrix in yellow.
Spaces, punctuation, and line endings are ignored because they have no serial-input mapping in `char_to_morse()`.

A valid serial character replaces any unfinished button character.
Its Morse pattern plays on the IO LEDs and buzzer.
A new valid serial character clears the pending IO LED playback and starts its own pattern; characters are not queued for complete sequential audio playback.

At 8 MHz, dots use approximately 1 kHz and dashes approximately 1.4 kHz.
Marks normally use a 50% PWM duty cycle, while the final mark uses approximately 10%.
Gaps are silent, and button-generated marks do not produce sound.

### Displays, fonts, and joystick

The left seven-segment digit shows the completed submission count modulo 16, including inserted word spaces and serial characters.
The right digit shows the current mark count from 1 to 9, shows `-` above 9, and is blank at zero.
The left decimal point is on while the right digit is active.

S1 changes the matrix font immediately: low selects three columns, and high selects five columns.
Adjacent glyphs have one blank column between them.

Tilt the joystick right to view older matrix history, or left to return towards the newest input.
Scrolling moves one column at a time and retains the newest 50 completed entries.
New input returns the view to the present.

On the Y axis, high ADC values increase brightness and low values decrease it.
The intended physical directions are up to brighten and down to darken; verify the axis orientation on your joystick.
Brightness starts at level 15 and stays within levels 1-15.
A tilt changes it immediately and then once per second while held; returning to neutral resets that repeat timer.
The deadzone uses hysteresis to reduce repeated actions caused by analogue noise.

## Software structure

```text
PlatformIO-ATmega324A/
|-- platformio.ini
|-- src/                       Application and peripheral modules
|-- docs/
|   |-- function_mindmaps.md    File, function, and call-flow diagrams
|   `-- function_map.png        Overview image
|-- include/                   PlatformIO placeholder
|-- lib/                       PlatformIO placeholder
`-- test/                      PlatformIO placeholder; no test suite
```

| Module | Responsibility |
| --- | --- |
| `morse.c` | Entry point, hardware setup, and input/output coordination |
| `buttons.c`, `sync_mode.c` | Button edges and timed-input state machine |
| `input_state.c`, `encoding.c` | In-progress Morse state, submission handling, and character conversion |
| `matrix_view.c` | Matrix history, previews, animation, scrolling, brightness, and EEPROM |
| `display.c`, `ledmatrix.c`, `spi.c` | Glyph data, matrix commands, and SPI transfers |
| `io_led.c`, `buzzer.c` | Beat queue, LED history, and PWM audio |
| `sevenseg.c` | Digit patterns and interrupt-driven multiplexing |
| `serial_view.c`, `terminalio.c`, `serialio.c` | Terminal layout, ANSI sequences, and buffered UART IO |
| `joystick.c`, `timer.c` | ADC readings, joystick thresholds, and shared timing |

`morse.c` contains `main()`.
Do not add another source file with a second entry point.
The main loop polls inputs and advances queued animation ticks; timer and UART interrupts handle their periodic or buffered work.
ADC conversions, SPI transfers, and EEPROM operations use synchronous hardware access.

See [the function diagrams](docs/function_mindmaps.md) for a more detailed navigation guide.

## EEPROM history

Each submitted entry stores a sequence number, character, colour, and validity marker.
Writes rotate through 64 slots, spreading repeated writes across the ring.
On start-up, the firmware finds the newest valid record and restores up to 50 entries in chronological order.
Incomplete previews, terminal cursor position, brightness, and the current session counter are not restored.

Power-cycle persistence depends on EEPROM surviving the programming process and the board's erase settings.
The storage format has no checksum or transactional commit mechanism, so interrupted writes are not guaranteed to recover correctly.

## Manual verification

There is no automated test suite in `test/`.
Use the following checks on the actual board after a successful build and upload.

| Check | Expected behaviour |
| --- | --- |
| Start-up | Splash appears; B0/B1/B2 enters the emulator without adding a mark |
| Enter `-.-.` with buttons | Preview progresses through T, N, K, C; submission becomes green |
| Submit twice after a character | Exactly one space is added; further submits do nothing |
| Hold one button and tap another | Each new rising edge is handled independently |
| Synchronous timing | Short and long B0 presses produce dot and dash; release gaps submit a character and then a space |
| Type `c`, `E`, and `7` | Uppercase yellow output and matching LED/buzzer patterns |
| Type punctuation | No submitted character or playback |
| Change S1 | Existing visible glyphs change width immediately |
| Submit over 50 entries | The newest 50 remain available through joystick scrollback |
| Move the Y axis | Brightness changes immediately, repeats once per second, and stays within bounds |
| Power-cycle the board | Saved matrix entries and colours return; the session counter resets |

Also inspect the known timing, speed, and capacity limitations below.
These checks are a verification guide, not a record of tests already passed.

## Known limitations

- **Animation timing:** the 100 ms tick is global and is not restarted for each input, so the first shift after submission may happen before a full 100 ms has elapsed.
- **Matrix traffic:** preview and submission paths redraw all 16 columns, using 160 SPI bytes per full redraw; this needs review against the assignment's command-efficiency requirements.
- **Scroll speed:** the speed calculation multiplies 16-bit unsigned values on AVR and can overflow, causing uneven speed changes and preventing the intended full-tilt speed.
- **Rapid tilt changes:** a larger X-axis tilt does not replace an already scheduled scroll deadline, so a pending slow step can delay the response to full tilt.
- **Long unsubmitted input:** the mark counter is eight bits and wraps after 255 marks, affecting the preview, submission logic, and seven-segment display.
- **Queue capacity:** the IO LED queue holds 48 beats and silently drops additional beats when full, which can truncate rapid input sequences.
- **Toolchain compatibility:** a review build with standalone AVR-GCC 14.1.0 failed because its ATmega324A headers did not recognise the `SPCR0`, `SPSR0`, and related names used in `spi.c`; this does not establish the result under PlatformIO or the laboratory toolchain.

For assessment, the current assignment document is the authority for required behaviour.
A PlatformIO build does not replace the required build and hardware demonstration under the laboratory's Microchip Studio 7 setup.
This README update did not perform a fresh PlatformIO build, upload, or board test.

## Credits

The original base-code headers credit Peter Sutton, Bradley Stone, and Ryan Wang.
The repository adds the input state, timed mode, display history, joystick controls, and sound modules described above.
The splash screen still contains the placeholder student name and number from the base code.
