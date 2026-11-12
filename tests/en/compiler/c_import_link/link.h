/* One program, two .c files imported by different SultanC modules: the
   prototypes every file sees. */
struct pt { int x; int y; };
typedef struct pt pt_t;

int twice(int v);               /* right.c */
int sum_pt(struct pt p);        /* right.c */
pt_t make_pt(int x, int y);     /* right.c */
int right_total(void);          /* right.c, calls left_base */
int left_base(void);            /* left.c */
int use_left(int v);            /* left.c, calls twice, sum_pt, make_pt */
