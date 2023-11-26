#include "link.h"

//#define SERIAL_DEBUG

struct Link *init_link()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_link");
#endif
    struct Link *p = (struct Link *)malloc(sizeof(struct Link));
    p->next = NULL;
    p->anchor_addr = 0;
    p->range = 0.0;

    return p;
}

void add_link(struct Link *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_link");
#endif
    struct Link *temp = p;
    //Find struct MyLink end
    while (temp->next != NULL) {
        temp = temp->next;
    }

    Serial.println("add_link:find struct MyLink end");
    //Create a anchor
    struct Link *a = (struct Link *)malloc(sizeof(struct Link));
    a->anchor_addr = addr;
    a->range = 0.0;
    a->dbm = 0.0;
    a->next = NULL;

    //Add anchor to end of struct MyLink
    temp->next = a;

    return;
}

struct Link *find_link(struct Link *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_link");
#endif
    if (addr == 0) {
        Serial.println("find_link:Input addr is 0");
        return NULL;
    }

    if (p->next == NULL) {
        Serial.println("find_link:Link is empty");
        return NULL;
    }

    struct Link *temp = p;
    //Find target struct MyLink or struct MyLink end
    while (temp->next != NULL) {
        temp = temp->next;
        if (temp->anchor_addr == addr) {
            return temp;
        }
    }

    Serial.println("find_link:Can't find addr");
    return NULL;
}

void fresh_link(struct Link *p, uint16_t addr, float range, float dbm)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_link");
#endif
    struct Link *temp = find_link(p, addr);

    if (temp != NULL) { 
        temp->range = range;
        temp->dbm = dbm;
        return;
    }
    else { 
        Serial.println("fresh_link:Fresh fail");
        return;
    }
}

void print_link(struct Link *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_link");
#endif
    struct Link *temp = p;

    while (temp->next != NULL) {
        char c[30];
        sprintf(c, "%X  %.2f  %.1f dbm", temp->next->anchor_addr, temp->next->range, temp->next->dbm);
        Serial.println(c);

        temp = temp->next;
    }

    Serial.println();

    return;
}

void delete_link(struct Link *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_link");
#endif
    if (addr == 0)
        return;

    struct Link *temp = p;
    while (temp->next != NULL) {
        if (temp->next->anchor_addr == addr) {
            struct Link *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }

        temp = temp->next;
    }
    return;
}

void make_link_json(struct Link *p, String *s)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_link_json");
#endif
    *s = "{\"links\":[";
    struct Link *temp = p;

    while (temp->next != NULL) {
        temp = temp->next;
        char link_json[50];
        sprintf(link_json, "{\"A\":\"%X\",\"R\":\"%.1f\"}", temp->anchor_addr, temp->range);
        *s += link_json;

        if (temp->next != NULL) {
            *s += ",";
        }
    }
    *s += "]}";
    // Serial.println(*s);
}