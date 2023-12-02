#include "IRReceiver.h"


int IRReceiver::pinNumber;
bool IRReceiver::interruptEnabled;

unsigned long IRReceiver::startTime;
unsigned long IRReceiver::highTime;
unsigned long IRReceiver::lowTime;

uint8_t IRReceiver::signalBuffer;
uint8_t IRReceiver::validBuffer;
uint8_t IRReceiver::bufferPosition;

bool IRReceiver::puceDetected;
Puce IRReceiver::pucePassed;
unsigned long IRReceiver::puceTime;
uint8_t IRReceiver::sectorFlag;

unsigned long IRReceiver::sectorTime;
unsigned long IRReceiver::lapTime;

bool IRReceiver::isActive;


void (*IRReceiver::_handleNewStart)() = nullptr;
void (*IRReceiver::_handleNewLap)(int) = nullptr;
void (*IRReceiver::_handleNewSector)(Puce, int) = nullptr;


IRReceiver::IRReceiver(int pinNum) {
    pinNumber = pinNum;
    interruptEnabled = false;

    startTime = 0;
    highTime = 0;
    lowTime = 0;

    signalBuffer = 0;
    validBuffer = 0;
    bufferPosition = 0b0;
    sectorFlag = 0b0;

    sectorTime = 0;
    lapTime = 0;

    isActive = false;
}

IRReceiver::~IRReceiver()
{
}

void IRReceiver::setupInterrupt()
{
    attachInterrupt(digitalPinToInterrupt(pinNumber), []() {
        if (digitalRead(pinNumber) == LOW) {
            highTime = micros() - startTime;
            startTime = micros();

            decodeBitHigh();

            return;
        }

        lowTime = micros() - startTime;
        startTime = micros();

        decodeBitLow();

        pucePassed = decodePuceBuffer();

        if (pucePassed != Puce::None) {
            puceDetected = true;
            puceTime = millis();
        }
        // On gère le cas ou la puce est None dans la loop (on est pas pressé) 
    }, CHANGE);

    interruptEnabled = true;
}

void IRReceiver::loop()
{
    if (isActive && millis() - sectorTime < MIN_SECTOR_TIME_MS)
        return;
    else if (!interruptEnabled)
        setupInterrupt();

    
    // aucune puce détectée -> on ne fait rien
    if (!puceDetected)
        return;

    puceDetected = false;

    if (pucePassed == Puce::None) {
        clearBuffer();
        return;
    }

    // on passe un secteur avant de passer la ligne de départ -> on ne fait rien
    if (pucePassed != Puce::Finish && !isActive) 
        return;


    // on a détecté une puce mais on ne l'a pas encore traitée


    detachInterrupt(digitalPinToInterrupt(pinNumber));
    interruptEnabled = false;
    clearBuffer();


    if (pucePassed == Puce::Finish) {
        if (!isActive) { // on vient de passer la ligne de départ pour la premiere fois
            isActive = true;

            if (_handleNewStart != nullptr)
                _handleNewStart();
        }
        else if (_handleNewLap != nullptr)
            _handleNewLap(puceTime - lapTime);

        sectorTime = puceTime;
        lapTime = puceTime;

        // On baisse tous les flags
        sectorFlag = 0b0;

        return;
    }

    if (_handleNewSector != nullptr)
        _handleNewSector(pucePassed, puceTime - sectorTime);

    // On lève le flag de la puce qui vient d'être détectée
    sectorFlag |= (1 << (uint8_t)pucePassed);

    sectorTime = puceTime;
}

void IRReceiver::clearBuffer()
{
    signalBuffer = 0;
    validBuffer = 0;
    bufferPosition = 0;
}

void IRReceiver::decodeBitHigh()
{
    if (abs((int32_t)highTime - HIGH_SHORT_TIME) < 100) {
        validBuffer |= bufferPosition; // on met le bit à 1
    
        if (HIGH_SHORT_BIT)
            signalBuffer |= bufferPosition; // on met le bit à 1
        else 
            signalBuffer &= ~bufferPosition; // on met le bit à 0
        }
    else if (abs((int32_t)highTime - HIGH_LONG_TIME) < 100) {
        validBuffer |= bufferPosition;
    
        if (HIGH_LONG_BIT)
            signalBuffer |= bufferPosition; // on met le bit à 1
        else 
            signalBuffer &= ~bufferPosition; // on met le bit à 0    
    }
    else {
        // bit non valide
        validBuffer &= ~bufferPosition; // on met le bit à 0
    }

    bufferPosition <<= 1;
}

void IRReceiver::decodeBitLow()
{
    if (abs((int32_t)lowTime - LOW_SHORT_TIME) < 100) {
        validBuffer |= bufferPosition;

        if (LOW_SHORT_BIT)
            signalBuffer |= bufferPosition; // on met le bit à 1
        else 
            signalBuffer &= ~bufferPosition; // on met le bit à 0
        }
    else if (abs((int32_t)lowTime - LOW_LONG_TIME) < 100) {
            validBuffer |= bufferPosition;

        if (LOW_LONG_BIT)
            signalBuffer |= bufferPosition; // on met le bit à 1
        else 
            signalBuffer &= ~bufferPosition; // on met le bit à 0
    }
    else {
        // bit non valide
        validBuffer &= ~bufferPosition; // on met le bit à 0
    }

    bufferPosition <<= 1;

    if (bufferPosition == 0b0)
        bufferPosition = 0b1;
}

Puce IRReceiver::decodePuceBuffer()
{
    if (validBuffer != 0b11111111)
        return Puce::None;

    const uint8_t maskFinish  = 0b00000000;
    const uint8_t maskSector1 = 0b01010101;
    const uint8_t maskSector2 = 0b10101010;
    const uint8_t maskSector3 = 0b11111111;

    switch (signalBuffer)
    {
    case maskFinish:
        return Puce::Finish;
        break;
    case maskSector1:
        return Puce::Sector1;
        break;
    case maskSector2:
        return Puce::Sector2;
        break;
    case maskSector3:
        return Puce::Sector3;
        break;
    default:
        break;
    }

    return Puce::None;
}

