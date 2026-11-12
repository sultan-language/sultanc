/* _BitInt wider than 128 bits: compiled code has no form for it. */
int wide(void) {
    unsigned _BitInt(200) z = 1;
    z <<= 70;
    return (int)(z >> 70);
}
