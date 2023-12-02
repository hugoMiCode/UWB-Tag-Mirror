#include <Arduino.h>

struct AnchorLinkNode
{
    uint16_t anchor_addr;
    float range;
    float dbm;
    struct AnchorLinkNode *next;
};

struct AnchorLinkNode *init_anchorLinkNode();
void add_link(struct AnchorLinkNode *p, uint16_t addr);
struct AnchorLinkNode *find_link(struct AnchorLinkNode *p, uint16_t addr);
void fresh_link(struct AnchorLinkNode *p, uint16_t addr, float range, float dbm);
void print_link(struct AnchorLinkNode *p);
void delete_link(struct AnchorLinkNode *p, uint16_t addr);
void make_link_json(struct AnchorLinkNode *p,String *s);

