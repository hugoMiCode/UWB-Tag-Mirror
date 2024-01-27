#include <WiFi.h>


class Emitter {
public:
    Emitter();
    Emitter(char* ssid, char* password, char* host, int port);
    ~Emitter();

    bool tryToConnectToWifi();
    bool tryToConnectToWifi(char* ssid, char* password);
    bool tryToConnectToHost();
    bool tryToConnectToHost(char* host, int port);

    void send(char* data);
    String read();

    bool isConnectedToWifi();


    IPAddress getLocalIP();
    char* getSSID();
    char* getPassword();

private:
    WiFiClient client;

    // wifi
    char* ssid = (char*)"SFR_1478";
    char* password = (char*)"85hacx7dhw49yetc9uwf";
    // char* ssid = (char*)"HUAWEI-HUGO";
    // char* password = (char*)"14159265";


    // char* host = (char*)WiFi.gatewayIP().toString().c_str();
    // char* host = (char*)"192.168.137.1";
    char* host = (char*)"192.168.1.61";
    int port = 82;

    const unsigned long timeout = 5000; // Timeout de la connection wifi en millisecondes
};