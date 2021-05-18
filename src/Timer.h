#ifndef VIRALINK_TIMER_H
#define VIRALINK_TIMER_H

#include "Arduino.h"

class Timer {

public:
    typedef void (*OnFinished)();

    long time;

    void start(unsigned long time, OnFinished onFinished);

    void loop();

    void stop();

    bool isRunning();

private:
    OnFinished onFinishedEvent;
    uint64_t startedTime;
    bool enable;
};

void Timer::start(unsigned long time, OnFinished onFinished) {
    this->time = time;
    onFinishedEvent = onFinished;
    startedTime = Uptime.getMilliseconds();
    enable = true;
}

void Timer::loop() {
    if (enable && (Uptime.getMilliseconds() - startedTime) > time) {
        enable = false;
        if (onFinishedEvent != nullptr) onFinishedEvent();
    }
}

void Timer::stop() {
    enable = false;
}

bool Timer::isRunning() {
    return enable;
}

#endif //VIRALINK_TIMER_H
