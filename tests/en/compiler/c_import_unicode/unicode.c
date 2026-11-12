/* C identifiers and literals beyond ASCII (C23 XID_Start/XID_Continue,
   universal character names, UTF-8 and UTF-16 literals). */
/* Bytes in a comment need not be UTF-8: caf�. */
#if 0
int skipped€ = 1; /* a group that is skipped is not tokenized */
#endif
int caf\u00e9 = 3;
int get(void) { return café + 39; }
int twice(int عدد) { return عدد * 2; }
int wide_length(void) {
    const unsigned short *u = u"a\U0001F600";
    int n = 0;
    while (u[n]) n++;
    return n;
}
int utf8_length(void) { return (int)sizeof(u8"é") - 1; }
