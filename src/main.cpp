#include <SPI.h>
#include "DW1000Ranging.h"

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "link.h"
// #include "IRReceiver.h"

#define TAG_ADDR "7D:00:22:EA:82:60:3B:9B"


#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

#define UWB_RST 27 // reset pin
#define UWB_IRQ 34 // irq pin
#define UWB_SS 21  // spi select pin

#define I2C_SDA 4
#define I2C_SCL 5

#define IR_RECEIVER_PIN 22


const char* ssid = "SFR_1478";
const char* password = "85hacx7dhw49yetc9uwf";
const char* host = "192.168.1.61";
WiFiClient client;

String all_json = "";

struct Link *uwb_data;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
// IRReceiver irReceiver(IR_RECEIVER_PIN);

void newRange();
void newDevice(DW1000Device *device);
void inactiveDevice(DW1000Device *device);
void logoshow(void);
void display_uwb(struct Link *p);


/// temporaire pour tester a remplacer par une structure comme link
// Puce lastPuce = Puce::None;
// int lastLapTime = 0;
// int lastSectorTime = 0;


void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }
    display.clearDisplay();

    logoshow();

    // init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(UWB_RST, UWB_SS, UWB_IRQ); // Reset, CS, IRQ pin
    // define the sketch as anchor. It will be great to dynamically change the type of module
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    // we start the module as a tag
    DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_SHORTDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_ACCURACY);


    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    if (client.connect(host, 82)) {
        Serial.println("Success");
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
    }

    uwb_data = init_link();

    // irReceiver.setupInterrupt();

    // irReceiver.attachNewStart([]() {
    //     lastPuce = Puce::Finish;
    //     lastLapTime = 0;
    //     lastSectorTime = 0;
    // });

    // irReceiver.attachNewLap([](int temps) {
    //     lastPuce = Puce::Finish;
    //     lastLapTime = temps;
    // });

    // irReceiver.attachNewSector([](Puce puce, int temps) {
    //     lastPuce = puce;
    //     lastSectorTime = temps;
    // });

    display.clearDisplay(); 
    display.display();
}

long int runtime = 0;

void loop()
{
    // irReceiver.loop();
    DW1000Ranging.loop();

    if ((millis() - runtime) > 200) {

        display_uwb(uwb_data);
        print_link(uwb_data);

        make_link_json(uwb_data, &all_json);
        if (client.connected()) {
            client.print(all_json);
            Serial.println("UDP send");
        }


        runtime = millis();
    }
}

void newRange()
{
    fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange(), DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
    Serial.print("ranging init; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);

    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);

    delete_link(uwb_data, device->getShortAddress());
}

// SSD1306

void logoshow(void)
{
    display.clearDisplay();

    display.setTextSize(2);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.println(F("Makerfabs"));

    display.setTextSize(1);
    display.setCursor(0, 20); // Start at top-left corner
    display.println(F("DW1000 DEMO"));
    display.display();
    delay(2000);
}

void display_uwb(struct Link *p)
{
    struct Link *temp = p;
    int row = 0;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    if (temp->next == NULL)
    {
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println("No Anchor");
        display.display();
        return;
    }

    while (temp->next != NULL)
    {
        temp = temp->next;

        char c[30];
        sprintf(c, "%X  %.2f  %.1f dbm", temp->anchor_addr, temp->range, temp->dbm);

        display.setTextSize(1);
        display.setCursor(0, row++ * 16); // Start at top-left corner
        display.print(c);

        if (row >= 3) { 
            break;
        }
    }


    //// pour tester si uwb fonctionne avec le IR
    // display.setTextSize(1);
    // display.setCursor(0, row++ * 16);
    // display.print("Lap: ");
    // display.print(lastLapTime);

    delay(100);
    display.display();
    return;
}
