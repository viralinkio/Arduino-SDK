#ifndef VIRALINK_PINCONTROLLER_TPP
#define VIRALINK_PINCONTROLLER_TPP

#include <Arduino.h>

class PinController {

public:
    enum LED_STATE {
        ON,
        OFF,
        TOGGLE
    };

    PinController(uint8_t, bool= false);

    void setLedStatus(LED_STATE);

private:
    uint8_t ledPin;
    bool activeLOW;
    bool lastMode;
};


void PinController::setLedStatus(LED_STATE state) {

    switch (state) {

        case OFF:
            digitalWrite(ledPin, activeLOW ? HIGH : LOW);
            lastMode = false;
            break;

        case ON:
            digitalWrite(ledPin, activeLOW ? LOW : HIGH);
            lastMode = true;
            break;

        case TOGGLE:
            digitalWrite(ledPin, !lastMode);
            lastMode = !lastMode;
            break;
        default:
            break;
    }
}

PinController::PinController(uint8_t ledPin, bool activeLow) {
    this->ledPin = ledPin;
    this->activeLOW = activeLow;
    pinMode(ledPin, OUTPUT);
    setLedStatus(OFF);
    lastMode = false;
}

#endif //VIRALINK_PINCONTROLLER_TPP
