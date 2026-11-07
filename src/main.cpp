#include <Arduino.h>

#include <mcp_can.h>
#include <SPI.h>
#include <VertivPsu.h>
#include "CurrentCalibration.h"

// Pin assignments
const int CAN0_CS_PIN = 53;
const int CAN0_INT_PIN = 9;

// Enter measured target current directly, or prefix it with A/I.
// V<volts> sets output voltage.
VertivPSU* charger = nullptr;
const float DEFAULT_AMPS = 6.0f;
const int DEFAULT_VOLTAGE_TENTHS = 585;
const float MIN_VOLTAGE = 32.0f;
const float MAX_VOLTAGE = 58.5f;
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

    if (command.startsWith("V")) {
        float newVoltage = command.substring(1).toFloat();
        if (newVoltage >= MIN_VOLTAGE && newVoltage <= MAX_VOLTAGE) {
            int voltageTenths = (int)(newVoltage * 10.0f + 0.5f);
            charger->setV(voltageTenths);
            Serial.print("Set voltage to ");
            Serial.print(voltageTenths / 10.0f, 1);
            Serial.println(" V");
        } else {
            Serial.println("Invalid voltage. Use 32.0-58.5 V");
        }
        return;
    }

    String measuredAmpsCommand = command;
    if (command.startsWith("A") || command.startsWith("I")) {
        measuredAmpsCommand = command.substring(1);
    }

    float measuredAmps = measuredAmpsCommand.toFloat();
    float selectedAmps;
    if (mapMeasuredAmpsToSelected(measuredAmps, selectedAmps)) {
        targetAmps = selectedAmps;
        charger->setCAmps(targetAmps);
        Serial.print("Target measured current: ");
        Serial.print(measuredAmps);
        Serial.print(" A; serial setting: ");
        Serial.print(targetAmps);
        Serial.println(" A");
    } else {
        Serial.println("Current target is outside the calibration range (4.15-24.0 A).");
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("alive");
    Serial.println("Enter a measured current in amps (e.g. 10), or V<volts> (e.g. V58.5).");

    charger = new VertivPSU(CAN0_CS_PIN);
    Serial.println("init Can0");
    charger->init(CAN0_INT_PIN);

    // Start with a safe default current and then allow runtime changes from the serial monitor.
    charger->setCAmps(DEFAULT_AMPS);
    charger->setV(DEFAULT_VOLTAGE_TENTHS);
}

void loop() {
    handleSerialCommands();
    charger->tick();
    charger->switchACpower(true);
    delay(50);
}
