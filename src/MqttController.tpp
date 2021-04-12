#ifndef VIRALINK_MQTT_CONTROLLER_TPP
#define VIRALINK_MQTT_CONTROLLER_TPP

#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <utility>
#include "viralink.h"

class MQTTController {
public:
    typedef void (*ConnectionEvent)();

    void updateSendAttributesInterval(float seconds);

    bool isViralinkConnected() const;

    void connectToViralink(NetworkController *networkController, String id, String token, String pass,
                           MQTT_CALLBACK_SIGNATURE, bool sendSystemAttributes = true,
                           ConnectionEvent connectionEvent = nullptr);

    void loop();

    PubSubClient *getMqttClient() const;

    virtual ~MQTTController() { delete mqttClient; }

private:
    PubSubClient *mqttClient;
    NetworkController *nc;
    float updateInterval = 10;
    unsigned long lastSendAttributes;
    unsigned long viralinkRecheckTimeout;
    bool sendAttributes;
    String id, pass, token;
    ConnectionEvent event;

    String get_Connection_info();

    void sendAttributesFunc();

};


bool MQTTController::isViralinkConnected() const {
    return mqttClient != nullptr && mqttClient->connected();
}

void MQTTController::connectToViralink(NetworkController *networkController, String Id, String viralinkToken,
                                       String viralinkPass,
                                       MQTT_CALLBACK_SIGNATURE, bool sendSystemAttributes,
                                       ConnectionEvent connectionEvent) {

    nc = networkController;
    this->sendAttributes = sendSystemAttributes;
    id = std::move(Id);
    token = std::move(viralinkToken);
    pass = std::move(viralinkPass);
    event = connectionEvent;

    delete mqttClient;
    mqttClient = new PubSubClient(*networkController->getClient());
    mqttClient->setServer(viralinkServerURL, 1883);
    mqttClient->setCallback(std::move(callback));
}

void MQTTController::loop() {

    if (mqttClient != nullptr) mqttClient->loop();

    if (sendAttributes && ((millis() - lastSendAttributes) > (updateInterval * 1000))) {
        lastSendAttributes = millis();
        sendAttributesFunc();
    }

    if ((millis() - viralinkRecheckTimeout) <= 2000) return;
    viralinkRecheckTimeout = millis();

    if (!isViralinkConnected() && nc != nullptr && nc->isConnected()) {

        printDBG("Connecting to Viralink ...");
        if (mqttClient->connect(id.c_str(), token.c_str(), pass.c_str())) {
            printDBGln("[Connected]");
            mqttClient->subscribe("v1/devices/me/attributes");
            if (sendAttributes)
                mqttClient->publish("v1/devices/me/attributes", get_Connection_info().c_str());
            if (event != nullptr) event();

        } else {
            printDBG("[FAILED] [ rc = ");
            printDBG(String(mqttClient->state()).c_str());
            printDBGln(" : retrying in 2 seconds]");
        }
    }
}

#ifdef ESP32
#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

#endif

String MQTTController::get_Connection_info() {
    StaticJsonDocument<1024> data;
    data[String("Cpu FreqMHZ")] = ESP.getCpuFreqMHz();
    data[String("Cpu SDK Version")] = ESP.getSdkVersion();
    data[String("Flash Chip Speed")] = ESP.getFlashChipSpeed();
    data[String("Viralink SDK Version")] = SDK_VERSION;
#ifdef ESP32
    data[String("Chip Type")] = "ESP32";
#endif

#ifdef ESP8266
    data[String("Chip Type")] = "ESP8266";
    data[String("Chip ID")] = ESP.getChipId();
#endif

    data[String("Connection Type")] =
            nc->getNetworkMode() == NetworkController::GSM ? "GSM" : "WIFI";
#ifdef F_GSM
    if (nc->getNetworkMode() == NetworkController::GSM) {
        TinyGsm *modem = nc->getModem();
        data[String("Modem Name")] = modem->getModemName();
        data[String("Modem Info")] = modem->getModemInfo();
        data[String("Location")] = modem->getGsmLocation();
        data[String("IMSI")] = modem->getIMSI();
        data[String("IMEI")] = modem->getIMEI();
        data[String("Operator")] = modem->getOperator();
    }
#endif
    char payload[2048];
    serializeJson(data, payload, sizeof(payload));
    String strPayload = String(payload);
    printDBGln(strPayload.c_str());
    return strPayload;
}

void MQTTController::sendAttributesFunc() {
    if (!isViralinkConnected())
        return;

    StaticJsonDocument<1024> data;
    data[String("upTime")] = millis() / 1000;
    data[String("ESP Free Heap")] = ESP.getFreeHeap();

#ifdef ESP32
    data[String("ESP32 temperature")] = (temprature_sens_read() - 32) / 1.8;
#endif

#ifdef F_GSM
    if (nc->getNetworkMode() == NetworkController::GSM) {
        TinyGsm *modem = nc->getModem();
        data[String("IP")] = modem->getLocalIP();
    }
#endif
#ifdef F_WIFI
    if (nc->getNetworkMode() == NetworkController::WIFI) {
        data[String("IP")] = nc->getWiFi()->localIP().toString();
    }
#endif
    char payload[1024];
    serializeJson(data, payload, sizeof(payload));
    mqttClient->publish("v1/devices/me/attributes", payload);
}

void MQTTController::updateSendAttributesInterval(float seconds) {
    updateInterval = seconds;
}

PubSubClient *MQTTController::getMqttClient() const {
    return mqttClient;
}

#endif