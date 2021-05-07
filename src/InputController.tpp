#ifndef VIRALINK_INPUT_CONTROLLER_TPP
#define VIRALINK_INPUT_CONTROLLER_TPP

#include <Arduino.h>

class InputController {

public:

    InputController(int pin, int detectThreshold);

    typedef void (*OnChange)(bool newState);

    void loop();

    void init(bool activeLow);

    void setOnChangeEvent(OnChange onChange) {
        InputController::onChangeEvent = onChange;
    }

    void trigger();

private:
    int pin;
    int detectThreshold;
    uint64_t firstDetectTime;
    bool state, recordState, activeLow;
    OnChange onChangeEvent;
public:
    bool isState() const;
};

void InputController::init(bool activeLow) {
    state = digitalRead(pin);
    recordState = state;
    this->activeLow = activeLow;
}

void InputController::loop() {
    bool newValue = digitalRead(pin);

    if (recordState != newValue) {
        recordState = newValue;
        firstDetectTime = Uptime::getMilliseconds();
    }

    if ((recordState != state) && (Uptime::getMilliseconds() - firstDetectTime) > detectThreshold) {
        state = recordState;
        if (onChangeEvent != nullptr) onChangeEvent((state != activeLow));
    }
}

bool InputController::isState() const {
    return state;
}

void InputController::trigger() {
    if (onChangeEvent != nullptr) onChangeEvent((state != activeLow));
}

InputController::InputController(int pin, int detectThreshold) : pin(pin),
                                                                 detectThreshold(detectThreshold) {}


#endif //VIRALINK_INPUT_CONTROLLER_TPP
