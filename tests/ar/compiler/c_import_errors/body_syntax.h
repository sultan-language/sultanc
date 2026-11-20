/* A statement with a missing operand inside a function body. */
static inline int broken_body(int value) {
    if (value > 1) { return value +; }
    return 0;
}
