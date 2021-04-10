#ifndef VIRALINK_RFCONTROLLER_TPP
#define VIRALINK_RFCONTROLLER_TPP

#include <RCSwitch.h>

class RFController {
public:

    typedef void (*KeyPressedEvent)(uint8_t);

    typedef void (*LearnedNewRemoteEvent)(unsigned long);

    RFController(uint8_t pin, int debounce_ms) : pin(pin), debounce_ms(debounce_ms) {}

    void init();

    void loop();

    void startLearning(LearnedNewRemoteEvent);

    void stopLearning();

    void onDataReceived(KeyPressedEvent);

    void setTrustAddress(unsigned long);

private:
    void callEvent(uint8_t);

    KeyPressedEvent event;
    LearnedNewRemoteEvent learnEvent;
    RCSwitch rcSwitch;
    uint8_t pin;
    unsigned long trustedAddress;
    bool learning;
    unsigned long lastReceiveTime;
    int debounce_ms;
};

void RFController::init() {
    rcSwitch.enableReceive(pin);
}

void RFController::loop() {
    if (rcSwitch.available()) {
        unsigned long value = rcSwitch.getReceivedValue();
        unsigned long rAddress = value & 16777200;
        uint8_t rKey = value & 15;

        if (!learning) {
            if (rAddress == trustedAddress && event != nullptr && (millis() - lastReceiveTime) > debounce_ms) {
                event(rKey);
                lastReceiveTime = millis();
            }

        } else {
            printDBG("Learned New Remote Control With Address: ");
            printDBGln(String(rAddress, HEX).c_str());
            setTrustAddress(rAddress);
            learning = false;
            if (learnEvent != nullptr) learnEvent(trustedAddress);
        }

        rcSwitch.resetAvailable();
    }
}

void RFController::setTrustAddress(unsigned long rAddress) {
    trustedAddress = rAddress;
}

void RFController::startLearning(LearnedNewRemoteEvent newRemoteEvent) {
    printDBGln("Started Learning New Remote Address Press Any Key on Remote Control");
    learning = true;
    learnEvent = newRemoteEvent;
}

void RFController::stopLearning() {
    learning = false;
}

void RFController::onDataReceived(RFController::KeyPressedEvent keyPressedEvent) {
    event = keyPressedEvent;
}

void RFController::callEvent(uint8_t key) {
    if (event != nullptr)
        event(key);
}

#endif //VIRALINK_RFCONTROLLER_TPP
