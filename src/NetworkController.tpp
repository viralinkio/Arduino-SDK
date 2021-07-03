#ifndef VIRALINK_NETWORK_CONTROLLER_TPP
#define VIRALINK_NETWORK_CONTROLLER_TPP

#include "PrintDBG.tpp"

#ifdef F_WIFI
#if defined(ESP32)

#include <WiFi.h>
#include <WebServer.h>

#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#else
#error "Not Supported Uptime Hardware"
#endif

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<h2>Viralink Config<h2>
<h3> Enter Your Config And Press Submit Button</h3>

<form action="/action_page">
  WiFi SSID:<br>
  <input type="text" name="wSSID">
  <br>
  WiFi Password:<br>
  <input type="text" name="wPASS">
  <br>
  GSM APN:<br>
  <input type="text" name="gAPN">
  <br>
  SIM PIn:<br>
  <input type="text" name="gPIN">
  <br>
  Device Token:<br>
  <input type="text" name="dTOKEN">
  <br><br>
  <input type="submit" value="Submit">
</form>
</body></html>)=====";

#endif

#ifdef F_GSM
#if !defined(GSM_ENABLE_PIN) || !defined(SerialAT)
#error "PlEASE DEFINE #GSM_ENABLE_PIN AND #SerialAT"
#endif

#include <TinyGsmClient.h>

#if (defined(ESP8266) || defined(ESP32)) && defined(TINY_GSM_MODEM_SIM800)

#include "SMS.tpp"

#endif

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

    void onConnecting(ConnectionEvent connectingFunction, float period_seconds);

    void onConnected(ConnectionEvent connectedFunction);

#ifdef F_GSM

    TinyGsm *getModem() const;

    void connect2GSM(String apn, String pin, int gsmTimeout_seconds = 30);

#if (defined(ESP8266) || defined(ESP32)) && defined(TINY_GSM_MODEM_SIM800)
    enum Status {
        AT_INDEX,
        ALL,
        REC_READ,
        REC_UNREAD,
        STO_SENT,
        STO_UNSENT,
    };

    bool deleteSMS(Status status, uint8_t index = 0);

    uint8_t readSMS(Status status, SMS buffer[], uint8_t index = 0);

    bool sendUTF16SMS(const String &number, const String &text);

#endif
#endif

#ifdef F_WIFI

    typedef String (*ConfigServerParameters)(String wifiSSID, String wifiPassword, String simAPN, String simPin,
                                             String viralinkToken);

    void wifiAPMode(const String &SSID, const String &PASS);

    WiFiClient *getWiFi();

    void connect2WIFi(String SSID, String PASS, int wifiTimeout_seconds = 30);

    void openConfigWebServer(ConfigServerParameters configServerParameters);

#endif

    bool isConnected() const;

    void autoConnect(String SSID, String PASS, String apn, String pin,
                     int wifiTimeout_seconds = 30, int gsmTimeout_seconds = 30);

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

    String gApn;
    String gSimPin;
    int gTimeout = -1;

#ifdef F_GSM
    TinyGsm *modem;
    TinyGsmClient *tinyGsmClient;

#endif

#ifdef F_WIFI
    static ConfigServerParameters serverParameters;
    WiFiClient wiFiClient;
#if defined(ESP32)
    static WebServer server;
#elif defined(ESP8266)
    static ESP8266WebServer server;
#else
#error "Not Supported Uptime Hardware"
#endif
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
#ifdef F_WIFI
    server.handleClient();
#endif
    uint64_t millis = Uptime.getMilliseconds();

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
            if ((millis - startWiFiConnectingTime) < ((uint64_t) (wTimeout * 1000))) {
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
    gApn = apn;
    gSimPin = pin;
    gTimeout = gsmTimeout_seconds;

#ifdef F_WIFI
    connect2WIFi(std::move(SSID), std::move(PASS), wifiTimeout_seconds);
#endif
}

#ifdef F_WIFI
NetworkController::ConfigServerParameters NetworkController::serverParameters;

#if defined(ESP32)
WebServer NetworkController::server(80);
#elif defined(ESP8266)
ESP8266WebServer NetworkController::server(80);

#else
#error "Not Supported Uptime Hardware"
#endif

void NetworkController::openConfigWebServer(ConfigServerParameters configServerParameters) {
    serverParameters = configServerParameters;

    //Which routine to handle at root location
    server.on("/", []() {
        server.send(200, "text/html", index_html); //Send web page
    });

    //form action is handled here
    server.on("/action_page", []() {
        String message = "";
        if (serverParameters != nullptr)
            message = serverParameters(server.arg("wSSID"), server.arg("wPASS"), server.arg("gAPN"), server.arg("gPIN"),
                                       server.arg("dTOKEN"));

        String s = message + "<br><a href=\"/\">Return to Home Page</a>";
        server.send(200, "text/html", s); //Send web page
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
    startWiFiConnectingTime = Uptime.getMilliseconds();
    networkMode = NOT_CONNECTED;

    printDBGln("Connecting to WIFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str(), PASS.c_str());
}

WiFiClient *NetworkController::getWiFi() {
    return &wiFiClient;
}

void NetworkController::setAutoReconnect(bool autoReconnectMode) {
    NetworkController::autoReconnect = autoReconnectMode;
}

#endif

#ifdef F_GSM

#if (defined(ESP8266) || defined(ESP32)) && defined(TINY_GSM_MODEM_SIM800)

bool NetworkController::deleteSMS(NetworkController::Status status, uint8_t smsIndex) {

    if (status == AT_INDEX && smsIndex == 0) return false;

    String cmgdValue;
    switch (status) {
        case AT_INDEX:
            cmgdValue = String(smsIndex) + ",0";
            break;
        case ALL:
            cmgdValue = "0,4";;
            break;
        case REC_READ:
            cmgdValue = "0,1";
            break;
        default:
            return false;
    }
    String bf;
    modem->sendAT(GF("+CMGF=1"));
    modem->waitResponse();

    modem->sendAT(GF("+CPMS=\"MT\""));
    modem->waitResponse();

    modem->sendAT(GF("+CMGD="), cmgdValue);
    modem->waitResponse(5000L, bf);

    bool result = bf.indexOf("OK") != -1;
    bf.clear();
    return result;
}

uint8_t NetworkController::readSMS(NetworkController::Status status, SMS *buffer, uint8_t smsIndex) {

    if (buffer == nullptr) return 0;
    if (status == AT_INDEX && smsIndex == 0) return 0;

    String cmglValue;
    switch (status) {
        case AT_INDEX:
            cmglValue = String(smsIndex);
            break;
        case ALL:
            cmglValue = "ALL";
            break;
        case REC_READ:
            cmglValue = "REC READ";
            break;
        case REC_UNREAD:
            cmglValue = "REC UNREAD";
            break;
        case STO_SENT:
            cmglValue = "STO SENT";
            break;
        case STO_UNSENT:
            cmglValue = "STO UNSENT";
            break;
    }
    String bf;
    modem->sendAT(GF("+CMGF=1"));
    modem->waitResponse();

    modem->sendAT(GF("+CSDH=1"));
    modem->waitResponse();

    modem->sendAT(GF("+CPMS=\"MT\""));
    modem->waitResponse();

    modem->sendAT(GF("+CMGL=\""), cmglValue, GF("\""));
    modem->waitResponse(10000L, bf);

    uint8_t size = 0;
    int index = -1;
    while (true) {
        index = bf.indexOf("+CMGL", index + 1);
        if (index == -1) break;
        int nextIndex = bf.indexOf("+CMGL", index + 1);

        String rawSMS = bf.substring(index, nextIndex != -1 ? nextIndex : bf.length() - 4);
        rawSMS.trim();
        buffer[size] = SMS(rawSMS);
        size++;
        if (nextIndex == -1)
            break;
        else index = nextIndex - 1;
    }
    bf.clear();
    return size;
}

bool NetworkController::sendUTF16SMS(const String &number, const String &text) {
    modem->sendAT(GF("+CMGF=1"));
    modem->waitResponse();
    modem->sendAT(GF("+CSCS=\"HEX\""));
    modem->waitResponse();
    modem->sendAT(GF("+CSMP=49,167,0,8"));
    modem->waitResponse();
    modem->sendAT(GF("+CMGS=\""), number, GF("\""));
    if (modem->waitResponse(GF(">")) != 1) return false;

    uint16_t size = text.length() / 2;
    String bf[size];
    SMS::stringToUTF16(text, bf);
    for (int i = 0; i < size; i++)
        modem->stream.print(bf[i]);
    modem->stream.write(static_cast<char>(0x1A));  // Terminate the message
    modem->stream.flush();
    return modem->waitResponse(60000L) == 1;
}

#endif

void NetworkController::connect2GSM(String apn, String pin, int gsmTimeout_seconds) {
    if (gsmConnecting)
        return;

    gApn = apn;
    gSimPin = pin;
    gTimeout = gsmTimeout_seconds;
    gsmConnecting = true;
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

TinyGsm *NetworkController::getModem() const {
    return modem;
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

#endif