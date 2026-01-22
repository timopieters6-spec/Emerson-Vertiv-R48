#include <Arduino.h>

#include <mcp_can.h>
#include <SPI.h>
#include <VertivPsu.h>

#define CAN0_CS 53

VertivPSU* charger = nullptr;

void setup() {
    Serial.begin(9600);
    Serial.println("alive");

    charger = new VertivPSU(SS);
    Serial.println("init Can0");
    charger->init(9);
}

void loop() {
    charger->tick();
    charger->setV(585);
    charger->updateVoltage();
    charger->switchACpower(true);
    delay(50);
}
