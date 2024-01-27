#include "Emitter.h"

// #define SERIAL_DEBUG

Emitter::Emitter()
{
    
}

Emitter::Emitter(char *ssid, char *password, char *host, int port)
{
    this->ssid = ssid;
    this->password = password;
    this->host = host;
    this->port = port;
}

Emitter::~Emitter()
{
}

bool Emitter::tryToConnectToWifi()
{
#ifdef SERIAL_DEBUG
    Serial.println("Triying to connect to wifi");
#endif
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        delay(500);
    #ifdef SERIAL_DEBUG
        Serial.print(".");
    #endif
    }
    
#ifdef SERIAL_DEBUG
    Serial.println("Connected to wifi");
#endif
    return false;
}

bool Emitter::tryToConnectToWifi(char *ssid, char *password)
{
    this->ssid = ssid;
    this->password = password;
    return tryToConnectToWifi();
}

bool Emitter::tryToConnectToHost()
{
#ifdef SERIAL_DEBUG
    Serial.println("Triying to connect to host");
#endif
    if (client.connect(host, port)) { 
    #ifdef SERIAL_DEBUG
        Serial.println("Connected to host");
    #endif
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
        return true;
    }

    return false;
}

bool Emitter::tryToConnectToHost(char *host, int port)
{
    this->host = host;
    this->port = port;
    return tryToConnectToHost();
}

void Emitter::send(char *data)
{
    if (client.connected()) {
        client.print(data);
    #ifdef SERIAL_DEBUG
        Serial.println("UDP send");
    #endif
    }
}

String Emitter::read()
{
    if (client.connected()) {
        String line = "";

        while (client.available()) {
            line = client.readStringUntil('\n');
        #ifdef SERIAL_DEBUG
            Serial.println(line);
        #endif
        }

        return line;
    }

    return "";
}

bool Emitter::isConnectedToWifi()
{
    return WiFi.status() == WL_CONNECTED;
}

IPAddress Emitter::getLocalIP()
{
    return WiFi.localIP();
}

char *Emitter::getSSID()
{
    return ssid;
}

char *Emitter::getPassword()
{
    return password;
}
