#ifndef VIRALINK_BUTTON_TPP
#define VIRALINK_BUTTON_TPP

class Button {
public:
    Button(uint8_t buttonPin, uint8_t mode, bool = false);

    typedef void (*ActionEvent)();

    void onLongClick(ActionEvent, unsigned int= 5000);

    void onDoubleClick(ActionEvent);

    void onClick(ActionEvent);

    void loop();

    void init(bool autoDetectDefaultMode = true);

private:
    uint8_t pin;
    uint8_t mode;
    bool activeLOW;
    ActionEvent longFunc;
    ActionEvent doubleFunc;
    ActionEvent clickFunc;

    unsigned long lastEventTime = 0;
    bool lastState = false;
    unsigned int longThreshold = 5000;
    unsigned int doubleClickDelayThreshold = 1000;
    unsigned int doubleClickClear = 1000;
    bool foundLong, firstClick;
    bool triggerClickEvent, triggerLongEvent, triggerDoubleEvent;
    unsigned long firstClickTime;

    void clickFunCall();

    void doubleFunCall();

    void longFunCall();

    void handleInterrupt();

    void attachInt();

    void detachInt();

    static void handleInterruptFunc(Button *b);

};

void IRAM_ATTR Button::handleInterruptFunc(Button *b) {
    if (b != nullptr)
        b->handleInterrupt();
}


void Button::init(bool autoDetectDefaultMode) {
    pinMode(pin, mode);
    if (autoDetectDefaultMode)
        digitalRead(pin) == LOW ? activeLOW = false : activeLOW = HIGH;
    attachInt();
}

void Button::attachInt() {
    attachInterruptArg(digitalPinToInterrupt(pin), reinterpret_cast<void (*)(void *)>(handleInterruptFunc), this,
                       activeLOW ? ONLOW : ONHIGH);
}

void Button::detachInt() {
    detachInterrupt(digitalPinToInterrupt(pin));
}

Button::Button(uint8_t buttonPin, uint8_t mode, bool activeLow) {
    pin = buttonPin;
    this->mode = mode;
    this->activeLOW = activeLow;
}

void Button::onLongClick(Button::ActionEvent longEvent, unsigned int time_ms) {
    this->longFunc = longEvent;
    this->longThreshold = time_ms;
}

void Button::onDoubleClick(Button::ActionEvent doubleEvent) {
    this->doubleFunc = doubleEvent;
}

void Button::onClick(Button::ActionEvent clickEvent) {
    clickFunc = clickEvent;
}

void Button::loop() {
    if (triggerClickEvent) clickFunCall();
    if (triggerDoubleEvent) doubleFunCall();
    if (triggerLongEvent) longFunCall();
}

void Button::handleInterrupt() {
    bool state = digitalRead(pin) == (activeLOW ? LOW : HIGH);
    if (state) {
        if (!foundLong && lastState && (millis() - lastEventTime) > longThreshold) {
            detachInt();
            triggerLongEvent = true;
            foundLong = true;
        }

    } else {
        if (lastState && (millis() - lastEventTime) > 20 && (millis() - lastEventTime) < 500) {
            triggerClickEvent = true;

            if (!firstClick) {
                firstClickTime = millis();
                firstClick = true;
            } else if ((millis() - firstClickTime) < doubleClickDelayThreshold) {
                firstClick = false;
                triggerDoubleEvent = true;
            }
        } else if (firstClick && ((millis() - firstClickTime) > doubleClickClear)) firstClick = false;
        else if (foundLong) foundLong = false;
    }


    if (state != lastState) {
        lastState = state;
        lastEventTime = millis();
    }
}

void Button::clickFunCall() {
    triggerClickEvent = false;
    if (clickFunc != nullptr) clickFunc();
}

void Button::doubleFunCall() {
    triggerDoubleEvent = false;
    if (doubleFunc != nullptr) doubleFunc();
}

void Button::longFunCall() {
    triggerLongEvent = false;
    if (longFunc != nullptr) longFunc();
    attachInt();
}

#endif //VIRALINK_BUTTON_TPP
