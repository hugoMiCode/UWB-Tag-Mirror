#include "AnchorLinkNode.h"

//#define SERIAL_DEBUG

struct AnchorLinkNode *init_anchorLinkNode()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_anchorLink");
#endif
    struct AnchorLinkNode *p = (struct AnchorLinkNode *)malloc(sizeof(struct AnchorLinkNode));
    p->next = NULL;
    p->anchor_addr = 0;
    p->range = 0.0;

    return p;
}

void add_link(struct AnchorLinkNode *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_anchorLink");
#endif
    struct AnchorLinkNode *temp = p;

    while (temp->next != NULL)
        temp = temp->next;

    //Create a anchor
    struct AnchorLinkNode *a = (struct AnchorLinkNode *)malloc(sizeof(struct AnchorLinkNode));
    a->anchor_addr = addr;
    a->range = 0.0;
    a->dbm = 0.0;
    a->next = NULL;

    //Add anchor to end of struct MyLink
    temp->next = a;

    return;
}

struct AnchorLinkNode *find_link(struct AnchorLinkNode *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_anchorLink");
#endif
    if (addr == 0) {
    #ifdef SERIAL_DEBUG
        Serial.println("find_anchorLink:Input addr is 0");
    #endif
        return NULL;
    }

    if (p->next == NULL) {
    #ifdef SERIAL_DEBUG
        Serial.println("find_anchorLink:Link is empty");
    #endif
        return NULL;
    }

    struct AnchorLinkNode *temp = p;

    while (temp->next != NULL) {
        temp = temp->next;

        if (temp->anchor_addr == addr)
            return temp;
    }

#ifdef SERIAL_DEBUG
    Serial.println("find_anchorLink:Can't find addr");
#endif
    return NULL;
}

void fresh_link(struct AnchorLinkNode *p, uint16_t addr, float range, float dbm)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_anchorLink");
#endif
    struct AnchorLinkNode *temp = find_link(p, addr);

    if (temp == NULL) {
    #ifdef SERIAL_DEBUG
        Serial.println("fresh_anchorLink:Can't find addr");
    #endif
        return;
    }

    temp->range = range;
    temp->dbm = dbm;

    return;
}

void print_link(struct AnchorLinkNode *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_anchorLink");
#endif
    struct AnchorLinkNode *temp = p;

    while (temp->next != NULL) {
        char c[30];
        sprintf(c, "%X  %.2f  %.1f dbm", temp->next->anchor_addr, temp->next->range, temp->next->dbm);
        Serial.println(c);

        temp = temp->next;
    }

    Serial.println();

    return;
}

void delete_link(struct AnchorLinkNode *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_anchorLink");
#endif
    if (addr == 0)
        return;

    struct AnchorLinkNode *temp = p;

    while (temp->next != NULL) {
        if (temp->next->anchor_addr == addr) {
            struct AnchorLinkNode *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }

        temp = temp->next;
    }

    return;
}

void make_link_json(struct AnchorLinkNode *p, String *s)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_anchorLink_json");
#endif
    struct AnchorLinkNode *temp = p;
    
    *s = "{\"AnchorLink\":[";

    while (temp->next != NULL) {
        temp = temp->next;

        char c[50];
        sprintf(c, "{\"A\":\"%X\",\"R\":\"%.1f\"}", temp->anchor_addr, temp->range);
        *s += c;

        if (temp->next != NULL)
            *s += ",";
    }
    *s += "]}";
    
    return;
}
