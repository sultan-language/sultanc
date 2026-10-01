/* Loads the root source unit. */

#include "core/program.h"
#include "core/program_loading.h"
#include "support/path/source_path.h"

/* Loads the program root. */
int __Program_Load_Root__(__Program__ *__Program_State__, const char *__Root_Path__)
{
    if (__Program_State__ == NULL || __Root_Path__ == NULL || __Root_Path__[0] == '\0')
    {
        return 0;
    }
    if (__Program_State__->__Project_Root__ == NULL)
    {
        __Program_State__->__Project_Root__ = __Source_Path_Canonical__(".");
        if (__Program_State__->__Project_Root__ == NULL)
        {
            return 0;
        }
    }
    return __Program_Load_File__(__Program_State__, __Root_Path__);
}
