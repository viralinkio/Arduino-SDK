#include <Arduino.h>

#define VIRALINK_DEFINES_H
#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define F_WIFI      // wifi feature
#define F_MQTT //add lib to connect to Platform via MQTT (you can use mqttClient anyWhere after import)

#define RELAY1_PIN 2
#define WIFI_SSID "wifissid"
#define WIFI_PASSWORD "wifipassword"
#define VIRALINK_TOKEN "xxxxxxxxxxx"

#include "viralink.h"

void on_message(const char *topic, byte *payload, unsigned int length);

void set_gpio_status(int pin, boolean enabled);

String get_gpio_status(uint8_t pins);

void connectToNetwork();

void connectToViralink();

NetworkController networkController;
MQTTController mqttController;

void setup() {
    SerialMon.begin(115200);
    pinMode(RELAY1_PIN, OUTPUT);

    networkController.init();

    networkController.onConnecting([](){
        digitalWrite(RELAY1_PIN,!digitalRead(RELAY1_PIN));
    },2);

    networkController.onConnected([]() {
        printDBG("Connected To: ");
        printDBGln(networkController.getNetworkMode() == NetworkController::WIFI ? "WIFI NETWORK " : "GSM NETWORK");
        connectToViralink(); // No Need to Check Viralink connection case it will reconnect if connection broke
    });
    connectToNetwork();
}

void loop() {
    networkController.loop();
    mqttController.loop();
}

void on_message(const char *topic, byte *payload, unsigned int length) {

    printDBGln("On message");
    char json[length + 1];
    strncpy(json, (char *) payload, length);
    json[length] = '\0';

    printDBG("Topic: ");
    printDBGln(topic);
    printDBG("Message: ");
    printDBGln(json);

    // Decode JSON request
    StaticJsonDocument<256> data;
    auto error = deserializeJson(data, (char *) json);
    if (error) {
        printDBG("deserializeJson() failed with code ");
        printDBGln(error.c_str());
        return;
    }

    // Check request method
    String methodName = String((const char *) data["method"]);
    if (methodName.equals("setGpioStatus")) {
        // Update GPIO status and reply it's state
        set_gpio_status(data["params"]["pin"], data["params"]["enabled"]);
        String responseTopic = String(topic);
        responseTopic.replace("request", "response");
        mqttController.getMqttClient()->publish(responseTopic.c_str(), get_gpio_status(data["params"]["pin"]).c_str());
    }

    // you can update updateSendAttributesInterval with updateInterval method (you should send RPC message to device from platform)
    //used for responses with format {"updateInterval":10} - subscribe updates format
    if (data.containsKey("updateInterval"))
        mqttController.updateSendAttributesInterval(data["updateInterval"]);
        //used for responses with format {"shared":{"updateInterval":10}} - request shared attributes format
    else if (data.containsKey("shared"))
        mqttController.updateSendAttributesInterval(data["shared"]["updateInterval"]);
}

//set GPIO status
void set_gpio_status(int pin, boolean enabled) {
    digitalWrite(pin, enabled ? HIGH : LOW);
    // Update pin attributes
    if (mqttController.isViralinkConnected())
        mqttController.getMqttClient()->publish("v1/devices/me/attributes", get_gpio_status(pin).c_str());
}

// GET GPIO Status json
String get_gpio_status(uint8_t pin) {
    StaticJsonDocument<1024> data;
    data[String(pin)] = digitalRead(pin) != 0;
    char payload[2048];
    serializeJson(data, payload, sizeof(payload));
    String strPayload = String(payload);
    printDBG("Get gpio status: ");
    printDBGln(strPayload.c_str());
    return strPayload;
}

void connectToNetwork() {
    // autoConnect function try to connect to WIFI if fails then it will try to connect throw gsm network
    networkController.setAutoReconnect(true); //if network fails it will try to reconnect;
    networkController.connect2WIFi(WIFI_SSID, WIFI_PASSWORD);
}

void connectToViralink() {
    printDBGln("Try To Connect viralink platform");
    mqttController.connectToViralink(&networkController, "ESP8266", VIRALINK_TOKEN, "", on_message,
                                     true,
                                     []() {
                                         PubSubClient *client = mqttController.getMqttClient();
                                         client->subscribe("v1/devices/me/rpc/request/+");

                                         // send current status after connection
                                         printDBGln("Sending current GPIO status ...");
                                         client->publish("v1/devices/me/attributes",
                                                         get_gpio_status(RELAY1_PIN).c_str());

                                         // subscribe to receive updates of shared attributes - response format: {"updateInterval":10}
                                         client->subscribe("v1/devices/me/attributes");

                                         // request shared attributes from viralink - response format: {"shared":{"updateInterval":10}}
                                         printDBGln("Request updateInterval shared attributes...");
                                         StaticJsonDocument<100> data;
                                         data["sharedKeys"] = "updateInterval";
                                         char payload[100];
                                         serializeJson(data, payload, sizeof(payload));
                                         printDBGln(String(payload).c_str());
                                         client->publish("v1/devices/me/attributes/request/1", String(payload).c_str());
                                     });
}
