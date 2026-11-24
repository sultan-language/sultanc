#include "first.h"
#include "second.h"

item_t first_item(int weight) {
    item_t it = { weight, 2 };
    return it;
}

int item_total(const item_t *it) { return it->weight * it->count; }
