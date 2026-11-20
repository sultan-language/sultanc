/* The record and typedef both headers below include. */
#ifndef ITEM_H
#define ITEM_H
struct item { int weight; int count; };
typedef struct item item_t;
#define ITEM_LIMIT 40
#endif
