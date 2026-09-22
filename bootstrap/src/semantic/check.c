#include "semantic/check.h"
#include "semantic/check_function_signatures.h"
#include "semantic/check_type_declarations.h"

int __Semantic_Check_Program__(__Semantic_Context__ *__Context__)
{
    if (__Context__ == NULL || __Context__->__Program__ == NULL)
    {
        return 0;
    }
    if (!__Semantic_Check_Type_Declarations__(__Context__))
    {
        return 0;
    }
    if (!__Semantic_Check_Function_Signatures__(__Context__))
    {
        return 0;
    }
    return __Semantic_Check_Entry_Point__(__Context__);
}
