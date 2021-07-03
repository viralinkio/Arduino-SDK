#ifndef VIRALINK_H
#define VIRALINK_H

#define SDK_VERSION "0.0.32"

#ifndef VIRALINK_DEFINES_H
#error PLEASE DEFINE VIRALINK_DEFINES_H
#endif

#define  VIRALINK_URL "console.viralink.io"
char viralinkServerURL[] = VIRALINK_URL;

#include <Arduino.h>
#include "Uptime.h"
#include "PrintDBG.tpp"
#include "PinController.tpp"
#include "InputController.tpp"
#include "PulseMaker.tpp"

#if defined(ESP32) || defined(ESP8266)
#include "Persistence.tpp"
#include "Queue.tpp"
#include "Button.tpp"
#endif

#if defined(F_WIFI) || defined(F_GSM)

#include "NetworkController.tpp"

#endif

#ifdef F_MQTT
#if !defined(F_WIFI) && !defined(F_GSM)
#error "Please Enable At Least One of F_WIFI or F_GSM to be able use MqttController Package Or uncoment F_MQTT"
#endif

#include "MqttController.tpp"

#endif

#ifdef F_RF
#include "RFController.tpp"
#endif


#endif //VIRALINK_H
