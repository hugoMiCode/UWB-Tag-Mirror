#include <Arduino.h>
#include <IRReceiver.h>

struct PuceLinkNode
{
    int lap;        // lap number
    Puce puce;      // puce number
    int time;       // time in ms
    struct PuceLinkNode *next;
};

struct PuceLinkNode *init_puceLinkNode();
void add_link(struct PuceLinkNode *p, Puce puce, int time, int lap);
struct PuceLinkNode *find_link(struct PuceLinkNode *p, Puce puce);
void fresh_link(struct PuceLinkNode *p, Puce puce, int time, int lap);
void print_link(struct PuceLinkNode *p);
void delete_link(struct PuceLinkNode *p, Puce puce);
void make_link_json(struct PuceLinkNode *p, String *s);
void reset_link(struct PuceLinkNode *p);
