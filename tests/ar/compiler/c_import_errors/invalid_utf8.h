/* The bytes C0 AF are an overlong encoding of '/', not UTF-8. */
const char *slash(void);
#define SLASH "À¯"
int n = sizeof(SLASH);
