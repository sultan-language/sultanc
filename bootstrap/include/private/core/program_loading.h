#ifndef SULTANC__CORE_PROGRAM_LOADING_H__
#define SULTANC__CORE_PROGRAM_LOADING_H__

#include "program.h"

int __Program_Has_Path__(const __Program__ *__Program_State__, const char *__Path__);

int __Program_Load_File__(__Program__ *__Program_State__, const char *__Path__);

#endif
