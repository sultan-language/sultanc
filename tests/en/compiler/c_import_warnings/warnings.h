/* A header whose import succeeds with warnings only. */
#ifndef WARNINGS_H
#define WARNINGS_H
#warning "warnings.h is a draft"
int answer(void);
/* No prototype: left out of the module, with a warning at its location. */
int legacy();
#endif
