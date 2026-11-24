/* Handles: structs a program sees only through pointers (a database and
   its statements), as sqlite3.h declares sqlite3 and sqlite3_stmt. */
#ifndef HANDLES_H
#define HANDLES_H
typedef struct db db;
typedef struct stmt stmt;
struct cursor;

db *db_open(int id);
int db_open_into(int id, db **out);
void db_close(db *d);
db *db_null(void);
int db_is_null(const db *d);
int db_id(const db *d);
stmt *db_prepare(db *d, int rows);
int stmt_step(stmt *s);
void stmt_finalize(stmt *s);
struct cursor *db_cursor(db *d);
int cursor_close(struct cursor *c);
/* C code calling C code through a function pointer that takes the handle. */
int db_each(db *d, int (*visit)(db *, int, void *), void *context);
int db_sum(db *d);
#endif
