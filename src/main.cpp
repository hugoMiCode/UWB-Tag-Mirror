#include <SPI.h>
#include "DW1000Ranging.h"

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "LinkNode/AnchorLinkNode.h"
#include "LinkNode/PuceLinkNode.h"
#include "Emitter.h"


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


void display_uwb(struct AnchorLinkNode *p);
void display_ir(struct PuceLinkNode *p);


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

    DW1000Ranging.attachNewRange([](){
        fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange(), DW1000Ranging.getDistantDevice()->getRXPower());
    });

    DW1000Ranging.attachNewDevice([](DW1000Device *device){
        add_link(uwb_data, device->getShortAddress());
    });

    DW1000Ranging.attachInactiveDevice([](DW1000Device *device){
        delete_link(uwb_data, device->getShortAddress());
    });

    // we start the module as a tag
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
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
        // TODO On doit partager les informations avec emitter
        // TODO eset les temps des secteurs apres les avoir envoyés avec la class Emiter

        fresh_link(ir_data, Puce::Finish, temps);

        uint8_t sectorFlag = irReceiver.getSectorFlag();

        for (uint i = 1; i < 4; i++)
            if ((sectorFlag & (1 << i)) == 0)
                delete_link(ir_data, Puce(i)); // On supprime les secteurs qui n'ont pas été détectés
            else
                fresh_link(ir_data, Puce(i), 0); // On reset le temps des secteurs qui ont été détectés
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
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initialisation");
    display.display();
}

long int runtime = 0;

void loop()
{
    irReceiver.loop();
    DW1000Ranging.loop();

    if ((millis() - runtime) > 100) {

        display.clearDisplay();
        display_uwb(uwb_data);
        display_ir(ir_data);
        // display.drawFastHLine(0, 32, 128, SSD1306_WHITE);
        // display.drawPixel(64, 20, SSD1306_WHITE);
        display.display();

        // print_link(uwb_data);
        // print_link(ir_data);

        // make_link_json(uwb_data, &all_json);
        // make_link_json(ir_data, &all_json);

        // emitter.send((char*)all_json.c_str());


        runtime = millis();
    }
}


// SSD1306

void display_uwb(struct AnchorLinkNode *p)
{
    struct AnchorLinkNode *tempAnchor = p;
    int row = 4;

    if (tempAnchor->next == NULL) {
        display.setTextSize(1);
        display.setCursor(0, row++ * 8);
        display.print("No Anchor");
    }

    while (tempAnchor->next != NULL) {
        tempAnchor = tempAnchor->next;

        char c[30];
        sprintf(c, "%X  %.2f  %d dbm", tempAnchor->anchor_addr, tempAnchor->range, int(tempAnchor->dbm));

        display.setTextSize(1);
        display.setCursor(0, row++ * 8); // Start at top-left corner
        display.print(c);

        if (row >= 8) { 
            break;
        }
    }

    return;
}

void display_ir(PuceLinkNode *p)
{
    struct PuceLinkNode *tempPuce = ir_data;
    int gridPos = 0;

    // Affichage du chrono du tour en cours
    char c[30];

    display.setTextSize(1);
    display.setCursor(64 * (gridPos % 2), 8 * int(gridPos++ / 2));
    sprintf(c, "Lap: %.1f", irReceiver.getCurrentClockLap() / 1000.);
    display.print(c);

    display.setTextSize(1);
    display.setCursor(64 * (gridPos % 2), 8 * int(gridPos++ / 2));
    sprintf(c, "Sec: %.1f", irReceiver.getCurrentClockSector() / 1000.);
    display.print(c);

    // Affichage des temps des secteurs et du tour en cours
    while (tempPuce->next != NULL) {
        tempPuce = tempPuce->next;

        char c[30];
        if (tempPuce->puce == Puce::Finish)
            sprintf(c, "L %.2f", tempPuce->time / 1000.);
        else
            sprintf(c, "%d %.2f", int(tempPuce->puce), tempPuce->time / 1000.);

        display.setTextSize(1);
        display.setCursor(64 * (gridPos % 2), 8 * int(gridPos++ / 2)); // Start at top-left corner
        display.print(c);

        if (gridPos >= 6)
            break;
    }


    return;
}
