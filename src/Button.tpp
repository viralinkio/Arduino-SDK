#ifndef VIRALINK_BUTTON_TPP
#define VIRALINK_BUTTON_TPP

class Button {
public:
    Button(uint8_t buttonPin, uint8_t mode);

    typedef void (*ActionEvent)();

    void onLongClick(ActionEvent);

    void onDoubleClick(ActionEvent);

    void onClick(ActionEvent);

    void loop();

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
    bool foundLong;
    bool firstClick;
    unsigned long firstClickTime;

    void clickFunCall();

    void doubleFunCall();

    void longFunCall();

};

Button::Button(uint8_t buttonPin, uint8_t mode) {
    pin = buttonPin;
    this->mode = mode;
    pinMode(pin, mode);
    digitalRead(pin) == LOW ? activeLOW = false : activeLOW = HIGH;
}

void Button::onLongClick(Button::ActionEvent longEvent) {
    this->longFunc = longEvent;
}

void Button::onDoubleClick(Button::ActionEvent doubleEvent) {
    this->doubleFunc = doubleEvent;
}

void Button::onClick(Button::ActionEvent clickEvent) {
    clickFunc = clickEvent;
}

void Button::loop() {
    bool state = digitalRead(pin) == (activeLOW ? LOW : HIGH);

    if (state) {
        if (!foundLong && lastState && (millis() - lastEventTime) > longThreshold) {
            longFunCall();
            foundLong = true;
        }

    } else {
        if (lastState && (millis() - lastEventTime) > 20 && (millis() - lastEventTime) < 500) {
            clickFunCall();

            if (!firstClick) {
                firstClickTime = millis();
                firstClick = true;
            } else if ((millis() - firstClickTime) < doubleClickDelayThreshold) {
                firstClick = false;
                doubleFunCall();
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
    if (clickFunc != nullptr)clickFunc();
}

void Button::doubleFunCall() {
    if (doubleFunc != nullptr) doubleFunc();
}

void Button::longFunCall() {
    if (longFunc != nullptr) longFunc();
}

#endif //VIRALINK_BUTTON_TPP
