#include <SPI.h>
#include "DW1000Ranging.h"

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "LinkNode/AnchorLinkNode.h"
#include "LinkNode/PuceLinkNode.h"
#include "Emitter.h"


#define SERIAL_DEBUG

#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23

#define UWB_RST 27 // reset pin
#define UWB_IRQ 34 // irq pin
#define UWB_SS  21 // spi select pin

#define I2C_SDA 4
#define I2C_SCL 5


#define TAG_ADDR "7D:00:22:EA:82:60:3B:9B"
#define IR_RECEIVER_PIN 26



String all_json = "";

struct AnchorLinkNode *uwb_data;
struct PuceLinkNode *ir_data;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
IRReceiver irReceiver(IR_RECEIVER_PIN);
Emitter emitter;


void newRange();
void newDevice(DW1000Device *device);
void inactiveDevice(DW1000Device *device);
void display_uwb(struct AnchorLinkNode *p);


void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }


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



    uwb_data = init_anchorLinkNode();
    ir_data = init_puceLinkNode();

    irReceiver.setupInterrupt();

    irReceiver.attachNewStart([]() {
        reset_link(ir_data);
        add_link(ir_data, Puce::Finish, 0);
    });

    irReceiver.attachNewLap([](int temps) {
        // Il y a "beaucoup" de chose a faire ici
        // TODO Si un secteur est manquant sur le tour et present dans le link on le supprime
        // TODO On doit partager les informations avec emitter
        fresh_link(ir_data, Puce::Finish, temps);

        // On supprime les secteurs qui n'ont pas été détectés
        uint8_t sectorFlag = irReceiver.getSectorFlag();

        for (uint i = 1; i < 8; i++) {
            if ((sectorFlag & (1 << i)) == 0) {
                delete_link(ir_data, Puce(i));
            }
        }
    });

    irReceiver.attachNewSector([](Puce puce, int temps) {
        // Si la puce n'est pas dans le link on l'ajoute
        if (find_link(ir_data, puce) == nullptr) {
            add_link(ir_data, puce, temps);
            return;
        }

        fresh_link(ir_data, puce, temps);
    });


    emitter.tryToConnectToWifi();

    if (emitter.isConnectedToWifi()) {
        Serial.print("Connected to wifi, IP address: ");
        Serial.println(emitter.getLocalIP().toString());
    } else {
        Serial.println("Not connected to wifi");
    }

    emitter.tryToConnectToHost();


    display.clearDisplay(); 
    display.display();
}

long int runtime = 0;

void loop()
{
    irReceiver.loop();
    DW1000Ranging.loop();

    if ((millis() - runtime) > 200) {

        display_uwb(uwb_data);
        // print_link(uwb_data);

        make_link_json(uwb_data, &all_json);
        
        emitter.send((char*)all_json.c_str());


        runtime = millis();
    }
}

void newRange()
{
    fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange(), DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
#ifdef SERIAL_DEBUG
    Serial.print("ranging init; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);
#endif
    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
#ifdef SERIAL_DEBUG
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);
#endif
    delete_link(uwb_data, device->getShortAddress());
}

// SSD1306

void display_uwb(struct AnchorLinkNode *p)
{
    struct AnchorLinkNode *tempAnchor = p;
    int row = 0;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // if (tempAnchor->next == NULL) {
    //     display.setTextSize(1);
    //     display.setCursor(0, row++);
    //     display.print("No Anchor");
    // }

    while (tempAnchor->next != NULL) {
        tempAnchor = tempAnchor->next;

        char c[30];
        sprintf(c, "%X  %.2f  %.1f dbm", tempAnchor->anchor_addr, tempAnchor->range, tempAnchor->dbm);

        display.setTextSize(1);
        display.setCursor(0, row++ * 16); // Start at top-left corner
        display.print(c);

        if (row >= 3) { 
            break;
        }
    }


    struct PuceLinkNode *tempPuce = ir_data;

    while (tempPuce->next != NULL) {
        tempPuce = tempPuce->next;

        char c[30];
        sprintf(c, "%d  %d", int(tempPuce->puce), tempPuce->time);

        display.setTextSize(1);
        display.setCursor(0, row++ * 16); // Start at top-left corner
        display.print(c);

        if (row >= 6) { 
            break;
        }
    }

    delay(100);
    display.display();
    return;
}
