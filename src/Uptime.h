#ifndef VIRALINK_UPTIME_H
#define VIRALINK_UPTIME_H

#include "Arduino.h"

class Uptime {

public:
    static uint64_t getMilliseconds();

    static unsigned long getSeconds();

private:
    static uint64_t milliseconds;
    static unsigned long seconds;
    static unsigned long counter;
    static unsigned long lastMillis;

    static void calculateTime();
};

unsigned long Uptime::seconds = 0;
unsigned long Uptime::counter = 0;
unsigned long Uptime::lastMillis = 0;
uint64_t Uptime::milliseconds = 0;

uint64_t Uptime::getMilliseconds() {
    calculateTime();
    return milliseconds;
}

unsigned long Uptime::getSeconds() {
    calculateTime();
    return seconds;
}

void Uptime::calculateTime() {
    unsigned long now = millis();
    if (now < lastMillis)
        counter++;
    lastMillis = now;

    milliseconds = 4294967295 * counter + now;
    seconds = milliseconds / 1000;
}

#endif //VIRALINK_UPTIME_H
