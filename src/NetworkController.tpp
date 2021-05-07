#ifndef VIRALINK_NETWORK_CONTROLLER_TPP
#define VIRALINK_NETWORK_CONTROLLER_TPP

#include "PrintDBG.tpp"

#ifdef F_WIFI
#ifdef ESP32

#include <WiFi.h>
#include <AsyncTCP.h>

#else

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>

#endif

#include <ESPAsyncWebServer.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Config Device Page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    WiFi SSID <input type="text" name="wSSID"><br>
    WiFi Password <input type="text " name="wPASS"><br>
    GSM APN <input type="text " name="gAPN"><br>
    SIM PIn <input type="text " name="gPIN"><br>
    Device Token <input type="text " name="dTOKEN"><br>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

#endif


#ifdef F_GSM
#if !defined(GSM_ENABLE_PIN) || !defined(SerialAT)
#error "PlEASE DEFINE #GSM_ENABLE_PIN AND #SerialAT"
#endif

#include <TinyGsmClient.h>

#include <utility>

#endif

class NetworkController {

public:
    enum NETWORK_MODE {
        GSM,
        WIFI,
        NOT_CONNECTED
    };

    void init();

    void loop();

    typedef void (*ConnectionEvent)();

    Client *getClient();

    NETWORK_MODE getNetworkMode() const;

    void onConnecting(ConnectionEvent, float);

    void onConnected(ConnectionEvent);

#ifdef F_GSM

    TinyGsm *getModem() const;

    void connect2GSM(String, String, int = 30);

#endif

#ifdef F_WIFI

    void wifiAPMode(const String &SSID, const String &PASS);

    WiFiClient *getWiFi();

    void connect2WIFi(String, String PASS, int wifiTimeout_seconds = 30);

    void openConfigWebServer(ArRequestHandlerFunction onRequest);

#endif

    bool isConnected() const;

    void autoConnect(String, String, String, String, int = 30, int = 30);

    void setAutoReconnect(bool autoReconnect);

private:
    NETWORK_MODE networkMode;
    uint64_t lastConnectingTime;
    ConnectionEvent connectingEvent;
    ConnectionEvent connectedEvent;
    float connectingEventPeriod;

    bool wifiConnecting, gsmConnecting, autoReconnect;
    bool autoConnectMode;
    short reconnectFailedThreshold = 3;
    uint64_t loopTimeout, autoReconnectTimeout;

    uint64_t startWiFiConnectingTime;
    int wTimeout = -1;
    String wSSID, wPASS;

    uint64_t startGsmConnectingTime;
    String gApn;
    String gSimPin;
    int gTimeout = -1;

#ifdef F_GSM
    TinyGsm *modem;
    TinyGsmClient *tinyGsmClient;

#endif

#ifdef F_WIFI
    WiFiClient wiFiClient;
    AsyncWebServer server = AsyncWebServer(80);
#endif

};

void NetworkController::init() {
    networkMode = NOT_CONNECTED;

#ifdef F_GSM
    pinMode(GSM_ENABLE_PIN, OUTPUT);
    digitalWrite(GSM_ENABLE_PIN, LOW);
    modem = new TinyGsm(SerialAT);
    tinyGsmClient = new TinyGsmClient(*modem);
    SerialAT.begin(115200);

    digitalWrite(GSM_ENABLE_PIN, HIGH);
    delay(1000);
    digitalWrite(GSM_ENABLE_PIN, LOW);
    printDBGln("Initializing modem->..");
    modem->restart();
    String modemInfo = modem->getModemInfo();
    printDBG("Modem Info: ");
    printDBGln(modemInfo.c_str());
#endif
#ifdef F_WIFI
    WiFi.mode(WIFI_OFF);
#endif
}

void NetworkController::loop() {
    uint64_t millis = Uptime::getMilliseconds();

    if (connectingEvent != nullptr && (gsmConnecting || wifiConnecting) &&
        ((millis - lastConnectingTime) > ((uint64_t) (connectingEventPeriod * 1000)))) {
        connectingEvent();
        lastConnectingTime = millis;
    }

    if ((millis - loopTimeout) <= 500) return;
    loopTimeout = millis;

    if (wifiConnecting || gsmConnecting) {
#ifdef F_WIFI

        if (wifiConnecting) {
            if ((millis - startWiFiConnectingTime) < (wTimeout * 1000)) {
                if (WiFi.status() == WL_CONNECTED) {
                    printDBG("Connected To WiFi with IP: ");
                    printDBGln(WiFi.localIP().toString().c_str());
                    networkMode = WIFI;
                    wifiConnecting = false;
                    autoReconnectTimeout = millis;
                    reconnectFailedThreshold = 3;
                    if (connectedEvent != nullptr) connectedEvent();
                }
            } else {
                wifiConnecting = false;
                printDBGln("Can Not Connect to WIFi");
                networkMode = NOT_CONNECTED;
                WiFi.mode(WIFI_OFF);
            }
        }
#endif

#ifdef F_GSM
        if (gsmConnecting && modem->isNetworkConnected()) {
            printDBG("Connecting to ");
            printDBG(String(gApn).c_str());
            if (!modem->gprsConnect(gApn.c_str())) {
                printDBGln(" fail");
                networkMode = NOT_CONNECTED;
                gsmConnecting = false;
                return;
            }
            printDBGln(" success");

            if (modem->isGprsConnected()) {
                printDBGln("GPRS connected");

                networkMode = GSM;
                autoReconnectTimeout = millis;
                reconnectFailedThreshold = 3;
                if (connectedEvent != nullptr) connectedEvent();
                gsmConnecting = false;
                return;
            }

            networkMode = NOT_CONNECTED;
            gsmConnecting = false;
        } else if (!isConnected() && autoConnectMode && !wifiConnecting && !gsmConnecting)
            connect2GSM(gApn, gSimPin, gTimeout);
#endif
        return; // if it is trying to connect no need to continue;
    }


    if (!autoReconnect)
        return;

    if ((millis - autoReconnectTimeout) < 5000) return;
    autoReconnectTimeout = millis;

    if (reconnectFailedThreshold > 0) reconnectFailedThreshold = isConnected() ? 3 : reconnectFailedThreshold - 1;

    if (reconnectFailedThreshold == 0) {
        printDBGln("Auto Reconnect Mode activated");
        if (autoConnectMode) {
            autoConnect(wSSID, wPASS, gApn, gSimPin, wTimeout, gTimeout);
            return;
        }
#ifdef F_WIFI
        if (wTimeout != -1) {
            connect2WIFi(wSSID, wPASS, wTimeout);
            return;
        }
#endif
#ifdef F_GSM
        if (gTimeout != -1) {
            connect2GSM(gApn, gSimPin, gTimeout);
            return;
        }
#endif
    }
}

void NetworkController::autoConnect(String SSID, String PASS, String apn, String pin,
                                    int wifiTimeout_seconds, int gsmTimeout_seconds) {
    if (wifiConnecting || gsmConnecting)
        return;

    autoConnectMode = true;
    gApn = std::move(apn);
    gSimPin = std::move(pin);
    gTimeout = gsmTimeout_seconds;

#ifdef F_WIFI
    connect2WIFi(std::move(SSID), std::move(PASS), wifiTimeout_seconds);
#endif
}

#ifdef F_WIFI

void NetworkController::openConfigWebServer(ArRequestHandlerFunction onRequest) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    server.on("/get", HTTP_GET, std::move(onRequest));
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
    server.begin();
}

void NetworkController::wifiAPMode(const String &SSID, const String &PASS) {
#ifndef AP_WIFI_ADDRESS
#define AP_WIFI_ADDRESS IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)
#endif
    WiFi.softAPConfig(AP_WIFI_ADDRESS);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID.c_str(), PASS.c_str());
    networkMode = WIFI;
}

void NetworkController::connect2WIFi(String SSID, String PASS, int wifiTimeout_seconds) {
    if (wifiConnecting)
        return;

    wSSID = SSID;
    wPASS = PASS;
    wTimeout = wifiTimeout_seconds;
    wifiConnecting = true;
    startWiFiConnectingTime = Uptime::getMilliseconds();
    networkMode = NOT_CONNECTED;

    printDBGln("Connecting to WIFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str(), PASS.c_str());
}

#endif

#ifdef F_GSM

void NetworkController::connect2GSM(String apn, String pin, int gsmTimeout_seconds) {
    if (gsmConnecting)
        return;

    gApn = apn;
    gSimPin = pin;
    gTimeout = gsmTimeout_seconds;
    gsmConnecting = true;
    startGsmConnectingTime = Uptime::getMilliseconds();
    networkMode = NOT_CONNECTED;

    if (!modem->isNetworkConnected()) {
        if (gSimPin && modem->getSimStatus() != 3)
            modem->simUnlock(gSimPin.c_str());

        printDBG("Waiting for network...");
        if (!modem->waitForNetwork(gTimeout * 1000)) {
            printDBGln(" fail");
            networkMode = NOT_CONNECTED;
            gsmConnecting = false;
            return;
        }

        printDBGln(" success");
        if (modem->isNetworkConnected()) {
            printDBGln("Network connected");
        }
    }
}

#endif

void NetworkController::onConnecting(ConnectionEvent connectingFunction, float period_seconds) {
    this->connectingEvent = connectingFunction;
    this->connectingEventPeriod = period_seconds;
}

void NetworkController::onConnected(ConnectionEvent connectedFunction) {
    this->connectedEvent = connectedFunction;
}

bool NetworkController::isConnected() const {
#ifdef F_WIFI
    if (networkMode == WIFI) return WiFi.isConnected();
#endif

#ifdef F_GSM
    if (networkMode == GSM) return modem->isGprsConnected();
#endif
    return false;
}

NetworkController::NETWORK_MODE NetworkController::getNetworkMode() const {
    return networkMode;
}

Client *NetworkController::getClient() {
#ifdef F_WIFI
    if (networkMode == WIFI) return &wiFiClient;
#endif
#ifdef F_GSM
    if (networkMode == GSM) return tinyGsmClient;
#endif
    return nullptr;
}

#ifdef F_GSM

TinyGsm *NetworkController::getModem() const {
    return modem;
}

#endif
#ifdef F_WIFI

WiFiClient *NetworkController::getWiFi() {
    return &wiFiClient;
}

void NetworkController::setAutoReconnect(bool autoReconnectMode) {
    NetworkController::autoReconnect = autoReconnectMode;
}

#endif

#endif