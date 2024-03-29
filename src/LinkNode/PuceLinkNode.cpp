#include "PuceLinkNode.h"

//#define SERIAL_DEBUG

struct PuceLinkNode *init_puceLinkNode()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_puceLink");
#endif
    struct PuceLinkNode *p = (struct PuceLinkNode *)malloc(sizeof(struct PuceLinkNode));
    p->next = NULL;
    p->puce = Puce::None;
    p->time = 0;
    p->lap = 0;

    return p;
}

void add_link(PuceLinkNode * p, Puce puce, int time, int lap)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_puceLink");
#endif
    PuceLinkNode *temp = p;

    while (temp->next != NULL)
        temp = temp->next;
        
    //Create a puce
    PuceLinkNode *a = (PuceLinkNode *)malloc(sizeof(PuceLinkNode));
    a->puce = puce;
    a->time = time;
    a->lap = lap;
    a->next = NULL;

    //Add puce to end of struct MyLink
    temp->next = a;

    return;
}

struct PuceLinkNode *find_link(PuceLinkNode *p, Puce puce)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_puceLink");
#endif
    if (puce == Puce::None) {
    #ifdef SERIAL_DEBUG
        Serial.println("find_puceLink:Input puce is 0");
    #endif
        return NULL;
    }

    if (p->next == NULL) {
    #ifdef SERIAL_DEBUG
        Serial.println("find_puceLink:Link is empty");
    #endif
        return NULL;
    }

    PuceLinkNode *temp = p;

    while (temp->next != NULL) {
        temp = temp->next;

        if (temp->puce == puce)
            return temp;
    }

#ifdef SERIAL_DEBUG
    Serial.println("find_puceLink:Can't find puce");
#endif
    return nullptr;
}

void fresh_link(PuceLinkNode *p, Puce puce, int time, int lap)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_puceLink");
#endif
    PuceLinkNode *temp = find_link(p, puce);
    
    if (temp == NULL) {
    #ifdef SERIAL_DEBUG
        Serial.println("fresh_puceLink:Can't find addr");
    #endif
        return;
    }

    temp->time = time;
    temp->lap = lap;

    return;
}

void print_link(PuceLinkNode *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_puceLink");
#endif
    PuceLinkNode *temp = p;
    int row = 0;

    while (temp->next != NULL) {
        temp = temp->next;
        Serial.print("row:");
        Serial.print(row);
        Serial.print(" puce:");
        Serial.print(int(temp->puce));
        Serial.print(" time:");
        Serial.println(temp->time);
        row++;
    }

    return;
}

void delete_link(PuceLinkNode *p, Puce puce)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_puceLink");
#endif
    if (puce == Puce::None)
        return;

    struct PuceLinkNode *temp = p;

    while (temp->next != NULL) {
        if (temp->next->puce == puce) {
            PuceLinkNode *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }

        temp = temp->next;
    }

    return;
}

void make_link_json(PuceLinkNode *p, String *link)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_puceLink_json");
#endif
    *link = "{\"PuceLink\":[";
    struct PuceLinkNode *temp = p;

    while (temp->next != NULL) {
        temp = temp->next;

        char c[50];
        if (temp->puce == Puce::Finish)
            sprintf(c, "{\"L\":\"%2d\",\"P\":\"%2d\",\"T\":\"%6d\"}", temp->lap, int(temp->puce), temp->time);
        else
            sprintf(c, "{\"P\":\"%2d\",\"T\":\"%6d\"}", int(temp->puce), temp->time);
        
        *link += c;

        if (temp->next != NULL)
            *link += ",";
    }
    *link += "]}";

    return;
}

void reset_link(PuceLinkNode *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("reset_puceLink");
#endif
    struct PuceLinkNode *temp = p;

    while (temp->next != NULL) {
        PuceLinkNode *del = temp->next;
        temp->next = del->next;
        free(del);
    }

    return;
}
