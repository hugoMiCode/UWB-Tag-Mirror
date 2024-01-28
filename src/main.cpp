#include <SPI.h>
#include "DW1000Ranging.h"

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "LinkNode/AnchorLinkNode.h"
#include "LinkNode/PuceLinkNode.h"
#include "Emitter.h"
#include "MessageDecoder.h"



#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23

#define UWB_RST 27 // reset pin
#define UWB_IRQ 34 // irq pin
#define UWB_SS  21 // spi select pin

#define I2C_SDA 4
#define I2C_SCL 5

#define TAG_ADDR "7D:00:22:EA:82:60:3B:9B"
#define IR_RECEIVER_PIN 36



struct AnchorLinkNode *uwb_data = nullptr;
struct PuceLinkNode *ir_data = nullptr;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
IRReceiver irReceiver(IR_RECEIVER_PIN);
Emitter emitter;
MessageDecoder messageDecoder;




void init_display();
void init_uwb();
void init_irReceiver();
void init_emitter();
void init_messageDecoder();

void display_uwb(struct AnchorLinkNode *p);
void display_ir(struct PuceLinkNode *p);


void setup()
{
    Serial.begin(115200);

    uwb_data = init_anchorLinkNode();
    ir_data = init_puceLinkNode();

    init_display();
    init_uwb();
    init_irReceiver();
    init_emitter();
    init_messageDecoder();
}

long int runtime = 0;

void loop()
{
    irReceiver.loop();
    DW1000Ranging.loop();

    // test pour voir comment je peux récupérer les données depuis le serveur
    String message = emitter.read();

    if (message != "") {
        Command cmd = messageDecoder.decodeMessage(message.c_str());
    }


    // Il faut envoyer les données moins souvent pour ne pas saturer le serveur
    // IrReceiver devrait envoyer les données a chaque fois qu'il y a un changement
    // UWB devrait envoyer les données a chaque fois qu'il y a un changement aussi 
    if ((millis() - runtime) > 200) {

        display.clearDisplay();
        display_uwb(uwb_data);
        display_ir(ir_data);
        display.display();

        // TO DO : Rajouter l’adresse du tag dans le protocole 
        String all_json = "";
        make_link_json(uwb_data, &all_json);
        // make_link_json(ir_data, &all_json); 

        emitter.send((char*)all_json.c_str());


        runtime = millis();
    }
}


// Fonctions d'initialisation
void init_display()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }
}

void init_irReceiver()
{
    irReceiver.setupInterrupt();

    irReceiver.attachNewReset([]() { // On arrête la course
        reset_link(ir_data);
    });

    irReceiver.attachNewStart([]() {
        reset_link(ir_data);
        add_link(ir_data, Puce::Finish, 0, 1); // On est dans le premier tour
    });

    irReceiver.attachNewLap([](int temps, int lap) {
        fresh_link(ir_data, Puce::Finish, temps, lap);

        uint8_t sectorFlag = irReceiver.getSectorFlag();

        for (uint i = 1; i < 4; i++)
            if ((sectorFlag & (1 << i)) == 0)
                delete_link(ir_data, Puce(i)); // On supprime les secteurs qui n'ont pas été détectés
            else
                fresh_link(ir_data, Puce(i), 0, 0); // On reset le temps des secteurs qui ont été détectés
    });

    irReceiver.attachNewSector([](Puce puce, int temps, int lap) {
        // Si la puce n'est pas dans le link on l'ajoute
        if (find_link(ir_data, puce) == nullptr) {
            add_link(ir_data, puce, temps, lap);
            return;
        }

        fresh_link(ir_data, puce, temps, lap);
    });
}

void init_uwb()
{
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(UWB_RST, UWB_SS, UWB_IRQ); // Reset, CS, IRQ pin

    DW1000Ranging.attachNewRange([](){
        fresh_link(uwb_data, 
            DW1000Ranging.getDistantDevice()->getShortAddress(), 
            DW1000Ranging.getDistantDevice()->getRange(), 
            DW1000Ranging.getDistantDevice()->getRXPower());
    });

    DW1000Ranging.attachNewDevice([](DW1000Device *device){
        add_link(uwb_data, device->getShortAddress());
    });

    DW1000Ranging.attachInactiveDevice([](DW1000Device *device){
        delete_link(uwb_data, device->getShortAddress());
    });


    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_FAST_LOWPOWER);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_SHORTDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_FAST_ACCURACY);
    // DW1000Ranging.startAsTag((char*)TAG_ADDR, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
}

void init_emitter()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Connection to wifi");
    char c0[30];
    display.setCursor(0, 8);
    sprintf(c0, "SSID: %s", emitter.getSSID());
    display.println(c0);
    display.setCursor(0, 16);
    sprintf(c0, "Password: %s", emitter.getPassword());
    display.println(c0);
    display.display();

    emitter.tryToConnectToWifi();


    delay(2000);

    if (emitter.isConnectedToWifi()) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Connected to wifi");
        display.setCursor(0, 8);
        char c1[30];
        sprintf(c1, "IP: %s", emitter.getLocalIP().toString());
        display.println(c1);
        display.display();
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Not connected to wifi");
        display.display();
    }

    sleep(2);

    if (emitter.tryToConnectToHost()) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Connected to host");
        display.display();
    }
    else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Not connected to host");
        display.display();
    }

    sleep(2);


    display.clearDisplay(); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initialisation done");
    display.display();
}

void init_messageDecoder()
{
    messageDecoder.attachCmdResetRace([]() {
        irReceiver.reset();
    });

    messageDecoder.attachCmdStartRace([]() {
        irReceiver.start();
    });
}

// Fonctions d'affichage
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
