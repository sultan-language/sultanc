/* Function bodies Jisr parses (and does not translate): statements,
   block-scope declarations that shadow file-scope names, GNU extensions. */
#ifndef BODIES_H
#define BODIES_H

typedef int count_t;
struct node { struct node *next; int value; };

static inline int sum_list(const struct node *head) {
    int total = 0;
    for (const struct node *n = head; n; n = n->next) total += n->value;
    return total;
}

static inline int shadowing(int count_t) {
    /* A parameter named like the typedef: an int here. */
    int x = count_t * 2;
    {
        typedef long count_t;
        struct node { long wide; };
        count_t y = (count_t)x;
        struct node local = { y };
        x = (int)local.wide;
    }
    switch (x) {
    case 0 ... 3: return -1;
    case 4: break;
    default: x += 1;
    }
    while (x > 10) { x -= 3; if (x == 12) goto done; }
    do { x++; } while (x < 5);
done:
    return x ? x : (x = 1, x);
}

static inline int computed_goto(int *p) {
    static void *table[] = { &&first, &&second };
    goto *table[*p & 1];
first:
    return 1;
second:
    return 2;
}

static inline int statement_expression(int a) {
    int b = ({ int t = a * 2; t + 1; });
    __asm__ volatile ("" ::: "memory");
    if ((b = b * 2) == 0) return 0;
    return b;
}

/* Declarations after the bodies: the file scope is intact. */
count_t after_bodies(count_t c);
struct node *first_node(void);
enum { AFTER_BODIES = sizeof(count_t) + sizeof(struct node) };
#define NODE_SIZE ((int)sizeof(struct node))

#endif
