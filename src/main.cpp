#include <Arduino.h>

#include <mcp_can.h>
#include <SPI.h>
#include <VertivPsu.h>

#define CAN0_CS 53

// This example uses the PSU over CAN bus and allows changing the output amperage
// from the serial monitor. Send commands in this format:
//   A30   -> set output current to 30 A
//   I12.5 -> set output current to 12.5 A
// Values must remain in the valid operating range for the connected PSU.
VertivPSU* charger = nullptr;
const float DEFAULT_AMPS = 30.0f;
float targetAmps = DEFAULT_AMPS;

// Reads one serial line and updates the PSU current when a command is received.
void handleSerialCommands() {
    if (!Serial.available()) {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0) {
        return;
    }

    if (command.startsWith("A") || command.startsWith("AMP") || command.startsWith("I")) {
        float newAmps = command.substring(1).toFloat();
        if (newAmps > 0.0f && newAmps <= 60.0f) {
            targetAmps = newAmps;
            charger->setCAmps(targetAmps);
            Serial.print("Set amperage to ");
            Serial.print(targetAmps);
            Serial.println(" A");
        } else {
            Serial.println("Invalid amperage. Use 0-60 A");
        }
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("alive");
    Serial.println("Usage: send A<value> to change amperage, e.g. A30 or I12.5");

    charger = new VertivPSU(SS);
    Serial.println("init Can0");
    charger->init(9);

    // Start with a safe default current and then allow runtime changes from the serial monitor.
    charger->setCAmps(DEFAULT_AMPS);
}

void loop() {
    handleSerialCommands();
    charger->tick();
    charger->setV(585);
    charger->updateVoltage();
    charger->switchACpower(true);
    delay(50);
}
