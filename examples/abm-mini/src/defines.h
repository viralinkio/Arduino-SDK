#ifndef VIRALINK_DEFINES_H
#define VIRALINK_DEFINES_H

#define VIRALINK_DEBUG // enable debug on SerialMon
#define SerialMon Serial // if you need DEBUG SerialMon should be defined

#define F_GSM       // gsm feature add to Libs
#define TINY_GSM_MODEM_SIM800     //GSM MODEL Depends on Each Device
#define GSM_ENABLE_PIN 26         //if you enable GSM feature the gsm pin should be defined
#define SerialAT Serial2          //if you enable GSM feature the SerialAT should be defined

#define F_WIFI      // wifi feature
#define AP_WIFI_SSID "RGBW35" //wifi ssid for ap mode in config web server
#define AP_WIFI_PASS "Vira-Afzar" //wifi pass for ap mode in config web server

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
#define DHTPIN 27
#define DHTTYPE DHT21   // DHT 21 (AM2301)

#endif //DEFINES_H
