#ifndef VIRALINK_EEPROM_H
#define VIRALINK_EEPROM_H

/*

 this library store key-value data in following format
 <prefix><key-value count><keySize[0]><keyPayload[0]><valueSize[0]><valuePayload[0]><keySize[1]><keyPayload[1]><valueSize[1]><valuePayload[1]>
 */

#include "map"
#include <EEPROM.h>
#include <WString.h>

class PersistenceClass {

public:

    void init(int maxSize = 512);

    String *getValue(String key);

    bool removeKey(String key, bool commitNow = false);

    bool put(String key, String value, bool commitNow = false);

    uint8_t keysCount();

    bool clear(bool commitNow = false);

    bool checkExistence(String key);

    uint16_t availableSpace();

    bool commit();

private:
    uint16_t size = 0, freeSpace = 0;

    std::map<String, String> keyValues;
    String prefix = "VIRA";


    bool loadFromEEPROM();

    bool writeEEPROM(uint16_t address, String data);

    bool writeEEPROM(uint16_t address, const byte data[], uint16_t len);

    String readEEPROM(uint16_t startAddress, uint16_t len);

    bool readEEPROM(uint16_t startAddress, uint16_t len, byte *buffer);

    void addStringToByteArray(const String &text, byte *buffer, uint16_t startIndex = 0);

};

bool PersistenceClass::checkExistence(String key) {
    auto it = keyValues.begin();
    for (int i = 0; i < keyValues.size(); i++) {
        if (it->first.equals(key)) return true;
        it++;
    }
    return false;
}

uint16_t PersistenceClass::availableSpace() {
    return freeSpace;
}

bool PersistenceClass::clear(bool commitNow) {
    keyValues.clear();
    if (commitNow) return commit();
    return true;
}

uint8_t PersistenceClass::keysCount() {
    return keyValues.size();
}

String *PersistenceClass::getValue(String key) {
    if (!checkExistence(key)) return nullptr;
    return &keyValues[key];
}

bool PersistenceClass::removeKey(String key, bool commitNow) {
    if (!checkExistence(key)) return false;
    keyValues.erase(key);
    if (commitNow) return commit();
    return true;
}

bool PersistenceClass::put(String key, String value, bool commitNow) {
    if (key.isEmpty()) return false;
    keyValues[key] = std::move(value);
    if (commitNow) return commit();
    return true;
}

void PersistenceClass::init(int maxSize) {
    this->size = maxSize;
    freeSpace = size;
    if (this->size > 512) size = 512;
    EEPROM.begin(size);
    keyValues.clear();
    loadFromEEPROM();
}

bool PersistenceClass::commit() {
    byte buffer[512];
    uint16_t p = 0;
    addStringToByteArray(prefix, buffer);

    p += prefix.length();
    buffer[p] = keyValues.size();
    p++;

    auto it = keyValues.begin();
    for (int i = 0; i < keyValues.size(); i++) {
        String key = it->first;
        String value = it->second;

        buffer[p] = key.length();
        p++;
        addStringToByteArray(key, buffer, p);
        p += key.length();

        buffer[p] = value.length();
        p++;
        addStringToByteArray(value, buffer, p);
        p += value.length();

        it++;
    }

    if (writeEEPROM(0, buffer, p)) {
        freeSpace = size - p - 1;
        return true;
    }
    return false;
}

bool PersistenceClass::loadFromEEPROM() {
    if (!readEEPROM(0, prefix.length()).equals(prefix)) return false;

    uint16_t p = 0;
    p += prefix.length();
    byte keysSize = EEPROM.read(p);
    p++;

    for (int i = 0; i < keysSize; i++) {
        byte keySize = EEPROM.read(p);
        p++;
        String key = readEEPROM(p, keySize);
        p += keySize;

        byte valueSize = EEPROM.read(p);
        p++;
        String value = readEEPROM(p, valueSize);
        p += valueSize;

        keyValues[key] = value;
    }
    freeSpace = size - p - 1;
    return true;
}

void PersistenceClass::addStringToByteArray(const String &text, byte *buffer, uint16_t startIndex) {
    uint32_t len = text.length();
    byte temp[len];
    text.getBytes(temp, len + 1);
    for (int i = 0; i < len; i++)
        buffer[startIndex + i] = temp[i];
}

bool PersistenceClass::writeEEPROM(uint16_t address, String data) {
    if (data.isEmpty()) return false;
    uint32_t len = data.length();
    if (address + len > size) return false;

    for (int i = 0; i < len; i++)
        EEPROM.write(address + i, data[i]);
    return EEPROM.commit();
}

bool PersistenceClass::writeEEPROM(uint16_t address, const byte *data, uint16_t len) {
    if (data == nullptr) return false;
    if (address + len > size) return false;
    for (int i = 0; i < len; i++)
        EEPROM.write(address + i, data[i]);
    return EEPROM.commit();
}

String PersistenceClass::readEEPROM(uint16_t startAddress, uint16_t len) {
    String data;
    for (int i = startAddress; i < startAddress + len; i++)
        data += (char) EEPROM.read(i);
    return data;
}

bool PersistenceClass::readEEPROM(uint16_t startAddress, uint16_t len, byte *buffer) {
    for (int i = 0; i < len; i++)
        buffer[i] = EEPROM.read(startAddress + i);
    return true;
}

PersistenceClass Persistence;
#endif //VIRALINK_EEPROM_H
