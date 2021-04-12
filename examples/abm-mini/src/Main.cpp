#include <Arduino.h>
#include "defines.h"
#include <viralink.h>

void on_message(const char *topic, byte *payload, unsigned int length);

void set_gpio_status(int pin, boolean enabled);

String get_gpio_status(uint8_t pins);

void connectToNetwork();

void connectToViralink();

NetworkController networkController;
MQTTController mqttController;
PinController ackLed(ACK_LED_PIN, false);

// from 0-130 is reserved for default function if you need space for your own data use after 130th byte
// if you need more than 40 character for token , set more than default size (default size = 130)
Persistence persistence(150);
Button resetButton(RESET_KEY_PIN, INPUT_PULLUP);

RFController rfController(RF_PIN, 200); // used to receive with RF-433Mhz remote control

void setup() {
    SerialMon.begin(115200);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    resetButton.init();
    persistence.init();
    networkController.init();
    rfController.init();

    // detect 3s long pressed on button to activate factory reset
    resetButton.onLongClick([]() {
        printDBGln("Factory Reset Function Called");
        persistence.set_configured(false);
        delay(1000); // this is necessary before restart otherwise it will run again after reboot
        ESP.restart();
    }, 3000);

    // Recover Trusted Key from Persistence after Reboot
    if (persistence.readEEprom(131) == "1") {
        rfController.setTrustAddress(strtol(persistence.readEEprom(133).c_str(), nullptr, 16));
    }

    // double click reset key to start Learning Remote Trusted Address
    resetButton.onDoubleClick([]() {
        rfController.startLearning([](unsigned long address) {
            persistence.writeEEprom(131, "1");
            persistence.writeEEprom(133, String(address, HEX));
        });
    });

    //control Relays with RF-433 MHZ
    rfController.onDataReceived([](uint8_t key) {
        if (key == 1) set_gpio_status(RELAY1_PIN, !digitalRead(RELAY1_PIN));
        else if (key == 2) set_gpio_status(RELAY2_PIN, !digitalRead(RELAY2_PIN));
        else if (key == 4) set_gpio_status(RELAY3_PIN, !digitalRead(RELAY3_PIN));
    });

    if (!persistence.is_configured()) {
        ackLed.setLedStatus(PinController::OFF);
        networkController.wifiAPMode(String(AP_WIFI_SSID).c_str(), String(AP_WIFI_PASS).c_str());

        networkController.openConfigWebServer([](AsyncWebServerRequest *request) {
            persistence.set_configured(true);
            persistence.set_wifi_SSID(request->getParam("wSSID")->value());
            persistence.set_wifi_PASS(request->getParam("wPASS")->value());
            persistence.set_gsm_apn(request->getParam("gAPN")->value()); //save pin to unlock sim if needed
            persistence.set_sim_pin(request->getParam("gPIN")->value());
            persistence.set_platform_token(request->getParam("dTOKEN")->value());
            request->send(200, "text/html",
                          "Your Configuration Saved And Device Will Restart in a second <br><a href=\"/\">Return to Home Page</a>");
            delay(1000);
            ESP.restart();
        });
        return;
    } else {

        //blink ack led each 0.3 to show that device is connecting
        networkController.onConnecting([]() {
            ackLed.setLedStatus(PinController::TOGGLE);
        }, 0.3);

        networkController.onConnected([]() {
            ackLed.setLedStatus(PinController::OFF);
            printDBG("Connected To: ");
            printDBGln(networkController.getNetworkMode() == NetworkController::WIFI ? "WIFI NETWORK " : "GSM NETWORK");
            connectToViralink(); // No Need to Check Viralink connection case it will reconnect if connection broke
        });
        connectToNetwork();
    }
}

void loop() {
    rfController.loop();
    resetButton.loop();
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
    // you can use  networkController.connect2GSM() or networkController.connect2WIFi() function
    networkController.setAutoReconnect(true); //if network fails it will try to reconnect;
//    networkController.connect2GSM(persistence.get_gsm_apn(), persistence.get_sim_pin(), 15);
    networkController.connect2WIFi(persistence.get_wifi_SSID(), persistence.get_wifi_PASS(), 10);
//    networkController.autoConnect(persistence.get_wifi_SSID(), persistence.get_wifi_PASS(),
//                                  persistence.get_gsm_apn(), persistence.get_sim_pin(), 15, 10);
//
}

void connectToViralink() {
    printDBGln("Try To Connect viralink platform");
    mqttController.connectToViralink(&networkController, "ESP8266", persistence.get_platform_token(), "", on_message,
                                     true,
                                     []() {
                                         PubSubClient *client = mqttController.getMqttClient();
                                         ackLed.setLedStatus(PinController::ON);
                                         client->subscribe("v1/devices/me/rpc/request/+");

                                         // send current status after connection
                                         printDBGln("Sending current GPIO status ...");
                                         client->publish("v1/devices/me/attributes",
                                                         get_gpio_status(RELAY1_PIN).c_str());
                                         client->publish("v1/devices/me/attributes",
                                                         get_gpio_status(RELAY2_PIN).c_str());
                                         client->publish("v1/devices/me/attributes",
                                                         get_gpio_status(RELAY3_PIN).c_str());

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
