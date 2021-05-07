#ifndef VIRALINK_PULSE_MAKER_TPP
#define VIRALINK_PULSE_MAKER_TPP

#include "Arduino.h"

class PulseMaker {

public:
    PulseMaker(int pin, unsigned int count, unsigned int enableDurationMs, unsigned int disableDurationMs);

    typedef void (*OnFinished)();

    void start(unsigned int disableDurationMs, OnFinished onFinished);

    void start(OnFinished onFinished);

    void start();

    void stop();

    void stop(bool setStatus);

    void loop();

    void setState(bool state);

    void init(bool isActiveLow){
        this->activeLow = isActiveLow;
    }

private:
    int pin;
    unsigned int count, counter;
    bool activeLow;
    unsigned int enableDuration_ms;
    unsigned int disableDuration_ms;
    uint64_t lastEvent;
    bool running;
    bool state;
    OnFinished onFinishedEvent;
public:
    bool isRunning() const;

};

void PulseMaker::start() {
    running = true;
    counter = count * 2;
    state = false;
}

void PulseMaker::start(unsigned int enableDurationMs, OnFinished onFinished) {
    this->enableDuration_ms = enableDurationMs;
    this->onFinishedEvent = onFinished;
    start();
}

void PulseMaker::start(PulseMaker::OnFinished onFinished) {
    this->onFinishedEvent = onFinished;
    start();
}

void PulseMaker::stop() {
    running = false;
}

void PulseMaker::stop(bool setStatus) {
    running = false;
    digitalWrite(pin, activeLow != setStatus);
}

void PulseMaker::loop() {
    if (running) {
        if (state && (Uptime::getMilliseconds() - lastEvent) >= enableDuration_ms)
            setState(false);
        else if (!state && (Uptime::getMilliseconds() - lastEvent) >= disableDuration_ms)
            setState(true);
    }
}

void PulseMaker::setState(bool state) {
    this->state = state;
    lastEvent = Uptime::getMilliseconds();
    digitalWrite(pin, activeLow != state);
    if (running && counter-- == 1) {
        if (onFinishedEvent != nullptr) onFinishedEvent();
        stop();
    }
}

PulseMaker::PulseMaker(int pin, unsigned int count, unsigned int enableDurationMs, unsigned int disableDurationMs)
        : pin(pin), count(count), enableDuration_ms(enableDurationMs), disableDuration_ms(disableDurationMs) {}

bool PulseMaker::isRunning() const {
    return running;
}

#endif //VIRALINK_PULSE_MAKER_TPP

