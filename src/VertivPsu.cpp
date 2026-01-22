/*
  VertivPSU.cpp - Library for controlling Emerzon / Vertiv R48-3000 Power Supply Unit.
  Created by Ant Nikrooz, July 2022
  Released into the public domain.
*/

#include "Arduino.h"
#include <mcp_can.h>
#include <SPI.h>
#include "VertivPsu.h"

void VertivPSU::init(int inr)
{
    Serial.println("mcpcan");
    if (inr)
        pinMode(inr, INPUT); // Configuring pin for /INT input
    PIN_INTERRUPT = inr;

    // this->_acoff = startOff;

    if (can0->begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("PSU MCP2515 Initialized Successfully!");
        can0->setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.
    }
    else
    {
        Serial.println("Error Initializing PSU MCP2515...");
    }

    // attachInterrupt(interrupt, MCP2515_ISR, FALLING); // start interrupt
    // updateSettings(); //cos we don't know what amps were before
    // then we'll wait 6sec before ticks
    sendSync();
}

VertivPSU::VertivPSU(int cs)
{
    can0 = new MCP_CAN(cs);
    Serial.println("mcpcan crested");
    _cs = cs;
}

void VertivPSU::setV(int vOut)
{
    _vout = vOut;
    if (!_initialising)
        updateVoltage();
}
void VertivPSU::setCPerc(int iOutP)
{
    _ioutp = iOutP;
    if (!_initialising)
        updateSettings();
}
void VertivPSU::setCAmps(float iOutA)
{
    _iouta = iOutA;
    if (!_initialising)
        updateAmps();
}
void VertivPSU::switchACpower(bool on)
{
    if (_acOff == on)
    {
        _acOff = !on;
        if (!_initialising)
            sendControl();
    }
}

void VertivPSU::setDebug(bool on)
{
    _debug = on;
}

void VertivPSU::setCanDebug(bool on)
{
    _candebug = on;
}

bool VertivPSU::isConnected()
{
    return !_initialising;
}

void VertivPSU::tick() // needs calling from loop

{

    if (can0->checkReceive() == 3) // If can0_INT pin is low, read receive buffer !digitalRead(PIN_INTERRUPT) &&
    {
        // Serial.println("CAn rec");
        candecode();
    }

    // reset if no comms
    if (millis() > _lastreceived + 20000)
        _acOff = 1;
    if (millis() > _lastreceived + 25000)
        _initialising = 1;
    // raise an alarm!?

    if (millis() > _looptime + (_initialising ? 10000 : 6000)) // since control last sent
    {
        _looptime = millis();
        if (_initialising)
        {
            sendSync();
            gimme5();
        }
        else
        {
            updateSettings();
        }
    }
}

void VertivPSU::updateSettings()
{

    // ask for info
    _txId = 0x0647FF83;
    uint8_t ask[8] = {0x03, 0xF0, 0x00, 0x08, 0x3F, 0x94, 0x62, 0x9B};
    sendcommand(_txId, ask);

    updateAmps();

    // voltage...
    updateVoltage();

    sendCurrentPerc(0x0607FF83);
    sendCurrentPerc(0x0677FF83);

    sendControl();
}

void VertivPSU::updateAmps()
{
    // calc & send iout Amps
    long unsigned int currh = 0x3f000000;
    int multiplier = 0x80;
    int dones = 0;
    float iouta = (_iouta / 5 > 1.0f) ? (_iouta / 5) : 1.0f;

    for (float ampsect = 2.0f; ampsect < 50.0f; ampsect *= 2.0f)
    {
        float min_val = (iouta < ampsect) ? iouta : ampsect;
        if (min_val > dones)
        {
            float ibit = min_val - dones;
            long unsigned int mbit = ibit * 0x10000 * multiplier;
            currh += mbit;
            dones = ampsect;
        }
        multiplier /= 2;
    }

    uint8_t mesg[8] = {0x03, 0xF0, 0x00, 0x1A, 0, 0, 0, 0};
    mesg[4] = (currh & 0xff000000) >> 24;
    mesg[5] = (currh & 0x00ff0000) >> 16;
    mesg[6] = (currh & 0x0000ff00) >> 8;
    mesg[7] = currh & 0x000000ff;
    _txId = 0x0607FF83;
    sendcommand(_txId, mesg);
}

void VertivPSU::updateVoltage()
{
    _txId = 0x0607FF83;
    uint8_t mesg[8] = {0x03, 0xF0, 0x00, 0x21, 0, 0, 0, 0};
    long unsigned int volth = (_vout - 320) * 0x40000 / 10;
    mesg[4] = 0x42; //(volth & 0xff000000) >> 24;
    mesg[5] = (volth & 0x00ff0000) >> 16;
    mesg[6] = (volth & 0x0000ff00) >> 8;
    mesg[7] = volth & 0x000000ff;
    sendcommand(_txId, mesg);
}

void VertivPSU::sendCurrentPerc(long unsigned int id)
{
    // curr %
    uint8_t mesg[8] = {0x03, 0xF0, 0x00, 0x22, 0, 0, 0, 0};

    long unsigned int perch = (_ioutp / 10 * 0x800000) + 0x3D000000;
    mesg[4] = (perch & 0xff000000) >> 24;
    mesg[5] = (perch & 0x00ff0000) >> 16;
    mesg[6] = (perch & 0x0000ff00) >> 8;
    mesg[7] = perch & 0x000000ff;
    sendcommand(id, mesg);
}

void VertivPSU::sendControl()
{
    // control bits...
    uint8_t msg[8] = {0, 0xF0, 0, 0x80, 0, 0, 0, 0};
    msg[2] = _dcOff << 7 | _fanFull << 4 | _flashLed << 3 | _acOff << 2 | 1;
    _txId = 0x06080783;
    sendcommand(_txId, msg);

    // TEMP: walk-in off!
    /*
        _txId = 0x0607FF83;
        //03 F0 00 2A 3F 80 00 00 //start interval 1s
        uint8_t msg2[8] = {0x03, 0xF0, 0, 0x2A, 0x3F, 0x80, 0, 0};
        sendcommand(_txId, msg2);*/

    _looptime = millis();
}

void VertivPSU::sendSync()
{
    _txId = 0x0707FF83;
    uint8_t msg[8] = {0x04, 0xF0, 0x01, 0x5A, 00, 00, 00, 00};
    sendcommand(_txId, msg);
}
void VertivPSU::sendSync2()
{;
    _txId = 0x0717FF83;
    uint8_t msg[8] = {0x04, 0xF0, 0x5A, 00, 00, 00, 00, 00};
    sendcommand(_txId, msg);
}

void VertivPSU::gimme5()
{
    _txId = 0x06080783;
    uint8_t msg[8] = {0x20, 0xF0, 00, 0x80, 00, 00, 00, 00};
    sendcommand(_txId, msg);
}

void VertivPSU::sendcommand(long unsigned int &id, uint8_t tx[8])
{
    if (!_talking)
        return;
    if (_candebug)
    {
        sprintf(_msgString, "%.8lX:", id);
        for (byte i = 0; i < 8; i++)
        {
            sprintf(_msgString, " %.2X", tx[i]);
        }
    }
    // set the first bit back to 1 for extended
    can0->sendMsgBuf(id, 1, 8, tx); // | 0x80000000, 0, 8, tx);
}

void VertivPSU::candecode()
{
    can0->readMsgBuf(&_rxId, &_len, _rxBuf); // Read data: len = data length, buf = data byte(s)
    if (_rxBuf[0] == 0x11)
    {
        // status
        _lastreceived = millis();
        voutput = (float)(uint16_t(_rxBuf[1] << 8) + uint16_t(_rxBuf[2])) / 1022;
        ioutput = (int)(uint16_t(_rxBuf[6] << 8) + uint16_t(_rxBuf[7])) * 2;
    }
    else
    {

        // print out the message
        if (_candebug)
        {
            if ((_rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
                sprintf(_msgString, "CHG: %.8lX:", (_rxId & 0x1FFFFFFF));
            else
                sprintf(_msgString, "CHG: %.3lX,false,0,%1d", _rxId, _len);

            if ((_rxId & 0x40000000) == 0x40000000)
            { // Determine if message is a remote request frame.
                sprintf(_msgString, " REMOTE REQUEST FRAME");
            }
            else
            {
                for (byte i = 0; i < _len; i++)
                {
                    sprintf(_msgString, " %.2X", _rxBuf[i]);
                }
            }
        }

        if (_rxId == 0x00860F8003 && _rxBuf[3] == 0x58)
        {
            // delay(200);
            if (_rxBuf[4] == 0x46)
            {
                sendCurrentPerc(0x0607FF83);
            }
            else
            {
                sendControl();
                gimme5();
            }
        }

        if (_rxBuf[3] == 0x5D && _initialising) // && _got5B)
        {
            _initialising = 0;
            delay(500);
            sendControl();
        }

        if (_rxBuf[3] == 0x5B)
            _got5B = 1;

        if (_rxBuf[3] == 0x23 && !_sentBothSyncs)
        {
            _sentBothSyncs = 1;
            sendSync();
            sendSync2();
        }
    }
}

int VertivPSU::getmAmps()
{
    return ioutput;
}