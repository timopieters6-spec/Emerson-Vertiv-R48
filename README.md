# Emerson Vertiv R48 PSU Control

This project controls an Emerson / Vertiv R48 power supply over the CAN bus using an Arduino-compatible controller and the MCP2515 CAN interface.

## Features

- Initializes the PSU on CAN bus
- Sets output voltage, including runtime changes from the serial monitor
- Enables AC power
- Calibrates runtime current commands against measured output current

## Hardware

- Arduino Mega / compatible board
- MCP2515 CAN controller connected to the PSU
- PSU CAN wiring connected as required by your board and harness

## Usage

1. Upload the firmware to the board.
2. Open the serial monitor at 9600 baud.
3. Enter the desired measured current in amps, or send a voltage command:
   - `10` targets 10 A measured output (the calibration interpolates the PSU setting)
   - `A10` and `I10` are also accepted as current commands
   - `V58.5` to set 58.5 V
4. The current target must be within the calibrated measured range of 4.15-24.0 A. Values between calibration points are linearly interpolated. Voltage commands accept 32.0-58.5 V and are rounded to the nearest 0.1 V.

Example:

```text
Enter a measured current in amps (e.g. 10), or V<volts> (e.g. V58.5).
Target measured current: 10.00 A; serial setting: 12.24 A
```

## Notes

- The calibration points are stored in `src/CurrentCalibration.cpp`. Recalibrate for your PSU and measurement setup.
- Current targets outside the calibration range are rejected rather than extrapolated.
- The code starts at 6 A and 58.5 V; both can be adjusted at runtime.

## Build

From the project root, run:

```bash
python -m platformio run
```
