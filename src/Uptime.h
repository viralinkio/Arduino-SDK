#ifndef VIRALINK_UPTIME_H
#define VIRALINK_UPTIME_H

#include "Arduino.h"

class UptimeClass {

public:
    uint64_t getMilliseconds();

    unsigned long getSeconds();

private:
    uint64_t milliseconds = 0;
    unsigned long seconds = 0;
    unsigned long counter = 0;
    unsigned long lastMillis = 0;

    void calculateTime();
};

uint64_t UptimeClass::getMilliseconds() {
    calculateTime();
    return milliseconds;
}

unsigned long UptimeClass::getSeconds() {
    calculateTime();
    return seconds;
}

void UptimeClass::calculateTime() {
    unsigned long now = millis();
#if defined(ESP32)
    if (xPortGetCoreID() == 1) {
        if (now < lastMillis)
            counter++;
        lastMillis = now;
    }

#else
    if (now < lastMillis)
        counter++;
    lastMillis = now;

#endif

    milliseconds = 4294967295 * counter + now;
    seconds = milliseconds / 1000;
}

UptimeClass Uptime;
#endif //VIRALINK_UPTIME_H
