/* Releases semantic context storage. */

#include "semantic/context_lifecycle.h"
#include "semantic/context.h"

#include <string.h>

/* Releases the semantic context. */
void __Semantic_Context_Destroy__(__Semantic_Context__ *__Context__)
{
    if (__Context__ == NULL)
    {
        return;
    }
    __Vector_Destroy__(&__Context__->__Types__);
    __Vector_Destroy__(&__Context__->__Functions__);
    __Vector_Destroy__(&__Context__->__Type_Owners__);
    memset(__Context__, 0, sizeof(*__Context__));
}
