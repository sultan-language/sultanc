/* Recognizes the language entry-point name. */

#include "semantic/context_collection.h"
#include "semantic/context.h"
#include "support/text/equality.h"

/* Checks whether the semantic is main name. */
int __Semantic_Is_Main_Name__(__Text_Slice__ __Name__)
{
    return __Text_Equals_Cstr__(__Name__, "main") || __Text_Equals_Cstr__(__Name__, "القلعة") ||
           __Text_Equals_Cstr__(__Name__, "id_6D61696E") ||
           __Text_Equals_Cstr__(__Name__, "id_D8A7D984D982D984D8B9D987");
}
