#ifndef VIRALINK_EEPROM_H
#define VIRALINK_EEPROM_H

/*

EEPROM addresses
    |init:0-1           1   // it shows it should be on AP mode and config

    |-wifi info
    |ssid:2-32          30
    |pass:33-63         30

    |-GSM Info
    |apn:64-74          10
    |pin:75-85          10


    |token:86-129        43

 */

#include <EEPROM.h>
#include <WString.h>

class Persistence {

public:

    Persistence(int = 130);

    void init();

    void set_configured(bool);

    void set_wifi_SSID(String);

    void set_wifi_PASS(String);

    void set_sim_pin(String);

    void set_gsm_apn(String);

    void set_platform_token(String);

    String get_wifi_SSID();

    String get_wifi_PASS();

    String get_sim_pin();

    String get_gsm_apn();

    String get_platform_token();

    bool is_configured();

    void writeEEprom(int, String);

    String readEEprom(int);

private:
    int size;

    bool initOk;
};

void Persistence::init() {
    if (size < 130) return;
    initOk = true;
    EEPROM.begin(size);
}

void Persistence::writeEEprom(int address, String data) {
    if (!initOk) {
        printDBGln("Please Enter Persistence Size More than 102 Byte");
        return;
    }

    int len = data.length();
    int dFlag = 0;
    EEPROM.write(address, static_cast<const uint8_t>(len));
    for (int i = address + 1; i < address + len + 1; i++) {
        EEPROM.write(i, static_cast<const uint8_t>(data[dFlag]));
        dFlag++;
    }
    EEPROM.commit();
}

String Persistence::readEEprom(int address) {
    if (!initOk) {
        printDBGln("Please Enter Persistence Size More than 102 Byte");
        return "";
    }

    int len = int(EEPROM.read(address));
    String data;
    for (int i = address + 1; i < address + len + 1; i++) {
        data += char(EEPROM.read(i));
    }
    return data;
}

void Persistence::set_configured(bool value) {
    writeEEprom(0, value ? "1" : "0");
}

void Persistence::set_wifi_SSID(String value) {
    writeEEprom(2, value);
}

void Persistence::set_wifi_PASS(String value) {
    writeEEprom(33, value);
}

void Persistence::set_gsm_apn(String value) {
    writeEEprom(64, value);
}

void Persistence::set_sim_pin(String value) {
    writeEEprom(75, value);
}

void Persistence::set_platform_token(String value) {
    writeEEprom(86, value);
}

String Persistence::get_wifi_SSID() {
    return readEEprom(2);
}

String Persistence::get_wifi_PASS() {
    return readEEprom(33);
}

String Persistence::get_sim_pin() {
    return readEEprom(75);
}

String Persistence::get_gsm_apn() {
    return readEEprom(64);
}

String Persistence::get_platform_token() {
    return readEEprom(86);
}

bool Persistence::is_configured() {
    return (readEEprom(0).toInt() == 1);
}

Persistence::Persistence(int size) : size(size) {}

#endif //VIRALINK_EEPROM_H
