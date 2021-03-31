#ifndef VIRALINK_H
#define VIRALINK_H

#ifndef VIRALINK_DEFINES_H
#error PLEASE DEFINE VIRALINK_DEFINES_H
#endif

#define  VIRALINK_URL "console.viralink.io"
char viralinkServerURL[] = VIRALINK_URL;

#include "PrintDBG.tpp"
#include "Persistence.tpp"
#include "PinController.tpp"
#include "Button.tpp"

#if defined(F_WIFI) || defined(F_GSM)

#include "NetworkController.tpp"

#endif

#ifdef F_MQTT

#include "MqttController.tpp"

#endif

#ifdef F_RF
#include "RFController.tpp"
#endif


#endif //VIRALINK_H
