# Emerson Vertiv R48 PSU Control

This project controls an Emerson / Vertiv R48 power supply over the CAN bus using an Arduino-compatible controller and the MCP2515 CAN interface.

## Features

- Initializes the PSU on CAN bus
- Sets output voltage
- Enables AC power
- Allows runtime amperage changes from the serial monitor

## Hardware

- Arduino Mega / compatible board
- MCP2515 CAN controller connected to the PSU
- PSU CAN wiring connected as required by your board and harness

## Usage

1. Upload the firmware to the board.
2. Open the serial monitor at 9600 baud.
3. Send a command to change the current, for example:
   - `A30` to set 30 A
   - `I12.5` to set 12.5 A
4. The PSU current is updated immediately if the value is valid.

Example:

```text
Usage: send A<value> to change amperage, e.g. A30 or I12.5
Set amperage to 30.00 A
```

## Notes

- The valid current range depends on the PSU model and configuration.
- Commands outside the supported range are rejected.
- The code starts at a default setting of 30 A and can be adjusted at runtime.

## Build

From the project root, run:

```bash
python -m platformio run
```
