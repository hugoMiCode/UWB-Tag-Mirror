#include <Arduino.h>


#define HIGH_SHORT_TIME 250
#define HIGH_LONG_TIME  500
#define LOW_SHORT_TIME  250
#define LOW_LONG_TIME   500

#define HIGH_SHORT_BIT 0
#define HIGH_LONG_BIT  1 
#define LOW_SHORT_BIT  0
#define LOW_LONG_BIT   1

#define MIN_SECTOR_TIME_MS 1000


enum class Puce{
  None =    0b11111111,
  Finish =  0b00,
  Sector1 = 0b01,
  Sector2 = 0b10,
  Sector3 = 0b11
};


class IRReceiver {
public:
    IRReceiver(int pinNum);
    ~IRReceiver();

    void setupInterrupt();
    void loop();
    void reset(); // Permet de finir la course en cours et de recommencer une nouvelle

    uint8_t getSectorFlag() {
        return sectorFlag;
    };

    unsigned long getCurrentClockSector() {
        if (isRacing)
            return millis() - sectorTime;

        return 0;
    };

    unsigned long getCurrentClockLap() {
        if (isRacing)
            return millis() - lapTime;

        return 0;
    };
    
    static void attachNewReset(void (*handleNewReset)()) {
        _handleNewReset = handleNewReset;
    };

    static void attachNewStart(void (*handleNewStart)()) {
        _handleNewStart = handleNewStart;
    };

    static void attachNewLap(void (*handleNewLap)(int, int)) {
        _handleNewLap = handleNewLap;
    };
    
    static void attachNewSector(void (*handleNewSector)(Puce, int, int)) {
        _handleNewSector = handleNewSector;
    };


private:
    void clearBuffer();

    // handler for new puce
    static void (*_handleNewReset)();
    static void (*_handleNewStart)();
    static void (*_handleNewLap)(int, int);
    static void (*_handleNewSector)(Puce, int, int);

    // static methods for interrupt
    static void decodeBitHigh();
    static void decodeBitLow();
    static Puce decodePuceBuffer();


private:
    static int irInputPin;
    static bool interruptEnabled;

    static unsigned long startTime;
    static unsigned long highTime;
    static unsigned long lowTime;

    static uint8_t signalBuffer;
    static uint8_t validBuffer;
    static uint8_t bufferPosition;

    static bool detectedPuceFlag;
    static Puce pucePassed;
    static unsigned long puceTime;
    static uint8_t sectorFlag;

    static unsigned long sectorTime;
    static unsigned long lapTime;
    static uint32_t lapNumber;

    static bool isRacing;
};
