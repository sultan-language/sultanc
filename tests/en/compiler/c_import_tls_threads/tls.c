/* A thread-local counter: each thread starts from the initial value. */
_Thread_local int counter = 3;

int bump(void) { return ++counter; }

static int seen;

/* Run on another thread: two bumps of that thread's own counter. */
void record(void) {
    bump();
    seen = bump();
}

int get_seen(void) { return seen; }
