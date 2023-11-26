#include <Arduino.h>

struct Link
{
    uint16_t anchor_addr;
    float range;
    float dbm;
    struct Link *next;
};

struct Link *init_link();
void add_link(struct Link *p, uint16_t addr);
struct Link *find_link(struct Link *p, uint16_t addr);
void fresh_link(struct Link *p, uint16_t addr, float range, float dbm);
void print_link(struct Link *p);
void delete_link(struct Link *p, uint16_t addr);
void make_link_json(struct Link *p,String *s);

