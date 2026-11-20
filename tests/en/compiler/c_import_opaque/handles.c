#include <stdlib.h>
#include "handles.h"

struct db { int id; int open; };
struct stmt { db *owner; int rows; int at; };
struct cursor { db *owner; };

db *db_open(int id) {
    db *d = malloc(sizeof *d);
    if (!d) return NULL;
    d->id = id;
    d->open = 1;
    return d;
}

int db_open_into(int id, db **out) {
    *out = db_open(id);
    return *out ? 0 : 1;
}

void db_close(db *d) { free(d); }
db *db_null(void) { return NULL; }
int db_is_null(const db *d) { return d == NULL; }
int db_id(const db *d) { return d ? d->id : -1; }

stmt *db_prepare(db *d, int rows) {
    stmt *s = malloc(sizeof *s);
    if (!s) return NULL;
    s->owner = d;
    s->rows = rows;
    s->at = 0;
    return s;
}

int stmt_step(stmt *s) { return s->at < s->rows ? ++s->at : 0; }
void stmt_finalize(stmt *s) { free(s); }

struct cursor *db_cursor(db *d) {
    struct cursor *c = malloc(sizeof *c);
    if (c) c->owner = d;
    return c;
}

int cursor_close(struct cursor *c) {
    int id = c->owner->id;
    free(c);
    return id;
}

int db_each(db *d, int (*visit)(db *, int, void *), void *context) {
    int i, total = 0;
    for (i = 1; i <= 3; i++) total += visit(d, i, context);
    return total;
}

static int add_id(db *d, int i, void *context) {
    (void)context;
    return d->id * i;
}

int db_sum(db *d) { return db_each(d, add_id, NULL); }
