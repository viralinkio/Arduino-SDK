#include <Arduino.h>

#define VIRALINK_DEFINES_H
#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define F_GSM       // gsm feature add to Libs
#define TINY_GSM_MODEM_SIM800     //GSM MODEL Depends on Each Device
#define GSM_ENABLE_PIN 26         //if you enable GSM feature the gsm pin should be defined
#define SerialAT Serial2          //if you enable GSM feature the SerialAT should be defined

#define F_WIFI      // wifi feature
#define AP_WIFI_SSID "Viralink" //wifi ssid for ap mode in config web server
#define AP_WIFI_PASS "Vira-Afzar" //wifi pass for ap mode in config web server
#define AP_WIFI_ADDRESS IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)

#define F_MQTT //add lib to connect to Platform via MQTT (you can use mqttClient anyWhere after import)
#define F_RF //add lib to Control RF 433Mhz

#define RESET_KEY_PIN 18
#define ACK_LED_PIN 19
#define BUZZER_PIN 23
#define RELAY1_PIN 32
#define RELAY2_PIN 33
#define RELAY3_PIN 25
#define IN1_PIN 34
#define IN2_PIN 39
#define IN3_PIN 36
#define RF_PIN 5
#define SENSOR_PIN 27

#include <viralink.h>
#include <DHT_U.h>
#include "Ticker.h"

#include <utility>

//DHT
DHT_Unified dht(SENSOR_PIN, DHT11);
sensor_t sensor;
uint64_t lastSentDHTTime; //used to send Temperature and Humidity To Server Periodically
Ticker restartTicker;

void resetESP() {
    ESP.restart();
}

void sendDHTParametersToViralink();

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
Button resetButton(RESET_KEY_PIN, INPUT_PULLDOWN);

RFController rfController(RF_PIN, 200); // used to receive with RF-433Mhz remote control

void mqttLoop(void *parameter) {
    while (true) {
        mqttController.loop();

        if ((Uptime.getMilliseconds() - lastSentDHTTime) > 10000) {
            lastSentDHTTime = Uptime.getMilliseconds();
            sendDHTParametersToViralink();
        }
        delay(1); // if you not delay watchDog will timeout
    }
}

void setup() {
    SerialMon.begin(115200);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    resetButton.init();
    Persistence.init();
    networkController.init();
    rfController.init();

    // Init DHT Sensor
    dht.begin();
    dht.temperature().getSensor(&sensor);

    // detect 3s long pressed on button to activate factory reset
    resetButton.onLongClick([]() {
        printDBGln("Factory Reset Function Called");
        Persistence.removeKey("configured", true);
        delay(1000); // this is necessary before restart otherwise it will run again after reboot
        ESP.restart();
    }, 3000);

    // Recover Trusted Key from Persistence after Reboot
    if (Persistence.checkExistence("RF_Address")) {
        rfController.setTrustAddress(strtol(Persistence.getValue("RF_Address")->c_str(), nullptr, 16));
    }

    // double click reset key to start Learning Remote Trusted Address
    resetButton.onDoubleClick([]() {
        rfController.startLearning([](unsigned long address) {
            Persistence.put("RF_Address", String(address, HEX), true);
        });
    });

    //control Relays with RF-433 MHZ
    rfController.onDataReceived([](uint8_t key) {
        if (key == 1) set_gpio_status(RELAY1_PIN, !digitalRead(RELAY1_PIN));
        else if (key == 2) set_gpio_status(RELAY2_PIN, !digitalRead(RELAY2_PIN));
        else if (key == 4) set_gpio_status(RELAY3_PIN, !digitalRead(RELAY3_PIN));
    });

    if (!Persistence.checkExistence("configured")) {
        ackLed.setLedStatus(PinController::OFF);
        networkController.wifiAPMode(String(AP_WIFI_SSID).c_str(), String(AP_WIFI_PASS).c_str());

        networkController.openConfigWebServer([](String wifiSSID, String wifiPassword, String simAPN, String simPin,
                                                 String viralinkToken) -> String {
            Persistence.put("configured", "true");
            Persistence.put("ssid", wifiSSID);
            Persistence.put("pass", wifiPassword);
            Persistence.put("apn", simAPN); //save pin to unlock sim if needed
            Persistence.put("pin", simPin);
            Persistence.put("token", viralinkToken);
            Persistence.commit();
            restartTicker.once(2, resetESP);
            return "Your Configuration Saved And Device Will Restart in 2 second";
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

    delay(1000);
    xTaskCreatePinnedToCore(
            mqttLoop, /* Function to implement the task */
            "MQTT_Loop", /* Name of the task */
            10000,  /* Stack size in words */
            NULL,  /* Task input parameter */
            0,  /* Priority of the task */
            NULL,  /* Task handle. */
            0); /* Core where the task should run */
}

void loop() {
    rfController.loop();
    resetButton.loop();
    networkController.loop();
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
        mqttController.addToPublishQueue(responseTopic.c_str(), get_gpio_status(data["params"]["pin"]).c_str());
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
    mqttController.addToPublishQueue("v1/devices/me/attributes", get_gpio_status(pin).c_str());
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
//    networkController.connect2GSM(Persistence.getValue("apn")->c_str(), Persistence.getValue("pin")->c_str(), 15);
    networkController.connect2WIFi(Persistence.getValue("ssid")->c_str(), Persistence.getValue("pass")->c_str(), 10);
//    networkController.autoConnect(Persistence.getValue("ssid")->c_str(), Persistence.getValue("pass")->c_str(),
//                                  Persistence.getValue("apn")->c_str(), Persistence.getValue("pin")->c_str(), 15, 10);
}

void connectToViralink() {
    printDBGln("Try To Connect viralink platform");
    mqttController.connectToViralink(&networkController, "ESP8266", Persistence.getValue("token")->c_str(), "",
                                     on_message,
                                     true,
                                     []() {
                                         PubSubClient *client = mqttController.getMqttClient();
                                         ackLed.setLedStatus(PinController::ON);
                                         client->subscribe("v1/devices/me/rpc/request/+");

                                         // send current status after connection
                                         printDBGln("Sending current GPIO status ...");
                                         mqttController.addToPublishQueue("v1/devices/me/attributes",
                                                                          get_gpio_status(RELAY1_PIN).c_str());
                                         mqttController.addToPublishQueue("v1/devices/me/attributes",
                                                                          get_gpio_status(RELAY2_PIN).c_str());
                                         mqttController.addToPublishQueue("v1/devices/me/attributes",
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
                                         mqttController.addToPublishQueue("v1/devices/me/attributes/request/1",
                                                                          String(payload).c_str());
                                     }, 4096);
}

void sendDHTParametersToViralink() {
    sensors_event_t eventH;
    sensors_event_t eventT;

    dht.temperature().getEvent(&eventT);
    dht.humidity().getEvent(&eventH);
    //if DHT is not working well return
    if (isnan(eventT.temperature) || isnan(eventH.relative_humidity))
        return;

    StaticJsonDocument<100> data;
    data["Temperature"] = eventT.temperature;
    data["Humidity"] = eventH.relative_humidity;
    char payload[100];
    serializeJson(data, payload, sizeof(payload));
    mqttController.addToPublishQueue("v1/devices/me/telemetry", String(payload).c_str());
}
