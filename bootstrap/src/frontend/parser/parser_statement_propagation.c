/* Expands the propagation operator `؟` / `?` (docs/en/reference/errors.md).
 *
 * Stage0 counterpart of compiler/frontend/desugar/propagation.sn: the same
 * four statement forms and the same expansion, built directly as AST. EXPR is
 * first bound to a hidden temporary (inferred type, like a call statement's
 * temporary), then matched:
 *
 *   tmp := EXPR;
 *   match tmp with
 *   | result.ok(v) -> { <dest := v; | return v; | (nothing)> }
 *   | result.err(e) -> { return result.err(e); };
 *
 * In a function whose declared result is written `اختيار …` the arms are
 * `option.some(v)` and `option.none -> { return option.none(); }`. The hidden
 * binding names contain `؟`, which no source identifier can. */

#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>
#include <string.h>

/* Builds a static text slice. */
static __Text_Slice__ __Propagation_Text__(const char *__Text__)
{
    __Text_Slice__ __Slice__;
    __Slice__.__Data__ = __Text__;
    __Slice__.__Length__ = strlen(__Text__);
    return __Slice__;
}

/* Allocates a pattern. */
static __Ast_Pattern__ *__Propagation_Pattern__(__Parser__ *__Parser_State__,
                                                __Ast_Pattern_Kind__ __Kind__,
                                                __Source_Span__ __Span__)
{
    __Ast_Pattern__ *__Pattern__ = (__Ast_Pattern__ *)__Ast_Allocate__(
        __Parser_State__->__Ast__, sizeof(*__Pattern__), alignof(__Ast_Pattern__));
    if (__Pattern__ == NULL)
    {
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    memset(__Pattern__, 0, sizeof(*__Pattern__));
    __Pattern__->__Kind__ = __Kind__;
    __Pattern__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    return __Pattern__;
}

/* `Type.Constructor` with zero or one payload pattern. */
static __Ast_Pattern__ *__Propagation_Enum_Pattern__(__Parser__ *__Parser_State__,
                                                     const char *__Type_Name__,
                                                     const char *__Constructor_Name__,
                                                     __Ast_Pattern__ *__Payload__,
                                                     __Source_Span__ __Span__)
{
    __Ast_Pattern__ *__Pattern__ =
        __Propagation_Pattern__(__Parser_State__, __Ast_Pattern_Enum__, __Span__);
    if (__Pattern__ == NULL)
    {
        return NULL;
    }
    __Pattern__->__As__.__Enum__.__Type_Name__ = __Propagation_Text__(__Type_Name__);
    __Pattern__->__As__.__Enum__.__Constructor_Name__ = __Propagation_Text__(__Constructor_Name__);
    if (__Payload__ != NULL)
    {
        __Pattern__->__As__.__Enum__.__Payloads__ =
            (__Ast_Pattern__ **)__Ast_Copy_Array__(__Parser_State__->__Ast__,
                                                   &__Payload__,
                                                   1U,
                                                   sizeof(__Payload__),
                                                   alignof(__Ast_Pattern__ *));
        if (__Pattern__->__As__.__Enum__.__Payloads__ == NULL)
        {
            (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            return NULL;
        }
        __Pattern__->__As__.__Enum__.__Payload_Count__ = 1U;
    }
    return __Pattern__;
}

/* An atom expression reading a named local. */
static __Ast_Expression__ *__Propagation_Name_Atom__(__Parser__ *__Parser_State__,
                                                     const char *__Name__,
                                                     __Source_Span__ __Span__)
{
    __Ast_Lvalue__ *__Lvalue__ = __Parser_New_Base_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base_Identifier__, __Propagation_Text__(__Name__), 0U, __Span__);
    __Ast_Expression__ *__Atom__ = NULL;
    if (__Lvalue__ == NULL)
    {
        return NULL;
    }
    __Atom__ = __Parser_New_Expression__(__Parser_State__, __Ast_Expression_Atom__, __Span__);
    if (__Atom__ == NULL)
    {
        return NULL;
    }
    __Atom__->__As__.__Atom__.__Kind__ = __Ast_Atom_Lvalue__;
    __Atom__->__As__.__Atom__.__As__.__Lvalue__ = __Lvalue__;
    return __Atom__;
}

/* `Type.Constructor(argument?)`. */
static __Ast_Expression__ *__Propagation_Construct__(__Parser__ *__Parser_State__,
                                                     const char *__Type_Name__,
                                                     const char *__Constructor_Name__,
                                                     __Ast_Expression__ *__Argument__,
                                                     __Source_Span__ __Span__)
{
    __Ast_Lvalue__ *__Type_Lvalue__ = __Parser_New_Base_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base_Identifier__, __Propagation_Text__(__Type_Name__), 0U, __Span__);
    __Ast_Lvalue__ *__Function__ = NULL;
    __Ast_Expression__ *__Call__ = NULL;
    if (__Type_Lvalue__ == NULL)
    {
        return NULL;
    }
    __Function__ = __Parser_New_Lvalue__(__Parser_State__, __Ast_Lvalue_Field__, __Span__);
    if (__Function__ == NULL)
    {
        return NULL;
    }
    __Function__->__As__.__Field__.__Parent__ = __Type_Lvalue__;
    __Function__->__As__.__Field__.__Field__ = __Propagation_Text__(__Constructor_Name__);
    __Call__ = __Parser_New_Expression__(__Parser_State__, __Ast_Expression_Call__, __Span__);
    if (__Call__ == NULL)
    {
        return NULL;
    }
    __Call__->__As__.__Call__.__Function__ = __Function__;
    if (__Argument__ != NULL)
    {
        __Call__->__As__.__Call__.__Arguments__ =
            (__Ast_Expression__ **)__Ast_Copy_Array__(__Parser_State__->__Ast__,
                                                      &__Argument__,
                                                      1U,
                                                      sizeof(__Argument__),
                                                      alignof(__Ast_Expression__ *));
        if (__Call__->__As__.__Call__.__Arguments__ == NULL)
        {
            (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            return NULL;
        }
        __Call__->__As__.__Call__.__Argument_Count__ = 1U;
    }
    return __Call__;
}

/* A block of zero or one statement. */
static __Ast_Block__ *__Propagation_Block__(__Parser__ *__Parser_State__,
                                            __Ast_Statement__ *__Statement__,
                                            __Source_Span__ __Span__)
{
    __Ast_Block__ *__Block__ = NULL;
    if (__Statement__ != NULL)
    {
        return __Parser_Block_From_Statement__(__Parser_State__, __Statement__);
    }
    __Block__ = __Parser_New_Block__(__Parser_State__, __Span__);
    if (__Block__ != NULL)
    {
        __Block__->__Statement_Count__ = 0U;
        __Block__->__Statements__ = NULL;
    }
    return __Block__;
}

/* A return statement. */
static __Ast_Statement__ *__Propagation_Return__(__Parser__ *__Parser_State__,
                                                 __Ast_Expression__ *__Value__,
                                                 __Source_Span__ __Span__)
{
    __Ast_Statement__ *__Statement__ =
        __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Return__, __Span__);
    if (__Statement__ != NULL)
    {
        __Statement__->__As__.__Return__ = __Value__;
    }
    return __Statement__;
}

int __Parser_Push_Propagation__(__Parser__ *__Parser_State__,
                                __Vector__ *__Statements__,
                                __Ast_Expression__ *__Value__,
                                __Parser_Propagation_Form__ __Form__,
                                __Ast_Lvalue__ *__Destination__,
                                __Source_Span__ __Mark__,
                                __Source_Span__ __Span__)
{
    static const char __Value_Name__[] = "؟قيمة";
    static const char __Error_Name__[] = "؟خطأ";
    int __Option__ = __Parser_State__->__Output_Is_Option__;
    __Temporary_Id__ __Temporary__ = __Parser_State__->__Next_Temporary__++;
    __Text_Slice__ __Empty__ = {NULL, 0U};
    __Ast_Statement__ *__Declaration__ = NULL;
    __Ast_Statement__ *__Copy__ = NULL;
    __Ast_Lvalue__ *__Temporary_Lvalue__ = NULL;
    __Ast_Lvalue__ *__Scrutinee_Lvalue__ = NULL;
    __Ast_Expression__ *__Scrutinee__ = NULL;
    __Ast_Pattern__ *__Ok_Payload__ = NULL;
    __Ast_Statement__ *__Ok_Statement__ = NULL;
    __Ast_Statement__ *__Error_Statement__ = NULL;
    __Ast_Match_Case__ __Cases__[2];
    __Ast_Statement__ *__Match__ = NULL;

    /* tmp := EXPR; */
    __Declaration__ =
        __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Variable_Declaration__, __Span__);
    if (__Declaration__ == NULL)
    {
        return 0;
    }
    __Declaration__->__As__.__Variable__.__Name_Kind__ = __Ast_Lvalue_Base_Temporary__;
    __Declaration__->__As__.__Variable__.__Name__.__Temporary__ = __Temporary__;
    __Declaration__->__As__.__Variable__.__Slot__.__Header__ =
        __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    __Declaration__->__As__.__Variable__.__Slot__.__Type__ = NULL;
    if (!__Parser_Push_Statement__(__Parser_State__, __Statements__, __Declaration__))
    {
        return 0;
    }
    __Temporary_Lvalue__ = __Parser_New_Base_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base_Temporary__, __Empty__, __Temporary__, __Span__);
    __Copy__ = __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Copy__, __Span__);
    if (__Temporary_Lvalue__ == NULL || __Copy__ == NULL)
    {
        return 0;
    }
    __Copy__->__As__.__Copy__.__Destination__ = __Temporary_Lvalue__;
    __Copy__->__As__.__Copy__.__Expression__ = __Value__;
    if (!__Parser_Push_Statement__(__Parser_State__, __Statements__, __Copy__))
    {
        return 0;
    }

    /* The ok arm. */
    if (__Form__ == __Parser_Propagation_Ignore__)
    {
        __Ok_Payload__ = __Propagation_Pattern__(__Parser_State__, __Ast_Pattern_Wildcard__, __Mark__);
    }
    else
    {
        __Ast_Expression__ *__Ok_Value__ =
            __Propagation_Name_Atom__(__Parser_State__, __Value_Name__, __Mark__);
        __Ok_Payload__ = __Propagation_Pattern__(__Parser_State__, __Ast_Pattern_Binding__, __Mark__);
        if (__Ok_Value__ == NULL || __Ok_Payload__ == NULL)
        {
            return 0;
        }
        __Ok_Payload__->__As__.__Binding__ = __Propagation_Text__(__Value_Name__);
        if (__Form__ == __Parser_Propagation_Assign__)
        {
            __Ok_Statement__ =
                __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Copy__, __Mark__);
            if (__Ok_Statement__ == NULL)
            {
                return 0;
            }
            __Ok_Statement__->__As__.__Copy__.__Destination__ = __Destination__;
            __Ok_Statement__->__As__.__Copy__.__Expression__ = __Ok_Value__;
        }
        else
        {
            __Ok_Statement__ = __Propagation_Return__(__Parser_State__, __Ok_Value__, __Mark__);
        }
        if (__Ok_Statement__ == NULL)
        {
            return 0;
        }
    }
    if (__Ok_Payload__ == NULL)
    {
        return 0;
    }
    __Cases__[0].__Pattern__ = __Propagation_Enum_Pattern__(__Parser_State__,
                                                            __Option__ ? "اختياري" : "نتيجة",
                                                            __Option__ ? "موجود" : "نجح",
                                                            __Ok_Payload__,
                                                            __Mark__);
    __Cases__[0].__Body__ = __Propagation_Block__(__Parser_State__, __Ok_Statement__, __Mark__);

    /* The error or absence arm returns it. */
    if (__Option__)
    {
        __Ast_Expression__ *__None__ =
            __Propagation_Construct__(__Parser_State__, "اختياري", "لا_شيء", NULL, __Mark__);
        if (__None__ == NULL)
        {
            return 0;
        }
        __Cases__[1].__Pattern__ =
            __Propagation_Enum_Pattern__(__Parser_State__, "اختياري", "لا_شيء", NULL, __Mark__);
        __Error_Statement__ = __Propagation_Return__(__Parser_State__, __None__, __Mark__);
    }
    else
    {
        __Ast_Pattern__ *__Error_Payload__ =
            __Propagation_Pattern__(__Parser_State__, __Ast_Pattern_Binding__, __Mark__);
        __Ast_Expression__ *__Error_Value__ =
            __Propagation_Name_Atom__(__Parser_State__, __Error_Name__, __Mark__);
        __Ast_Expression__ *__Error__ = NULL;
        if (__Error_Payload__ == NULL || __Error_Value__ == NULL)
        {
            return 0;
        }
        __Error_Payload__->__As__.__Binding__ = __Propagation_Text__(__Error_Name__);
        __Error__ = __Propagation_Construct__(__Parser_State__, "نتيجة", "خطأ", __Error_Value__, __Mark__);
        if (__Error__ == NULL)
        {
            return 0;
        }
        __Cases__[1].__Pattern__ = __Propagation_Enum_Pattern__(
            __Parser_State__, "نتيجة", "خطأ", __Error_Payload__, __Mark__);
        __Error_Statement__ = __Propagation_Return__(__Parser_State__, __Error__, __Mark__);
    }
    if (__Error_Statement__ == NULL)
    {
        return 0;
    }
    __Cases__[1].__Body__ = __Propagation_Block__(__Parser_State__, __Error_Statement__, __Mark__);
    if (__Cases__[0].__Pattern__ == NULL || __Cases__[0].__Body__ == NULL ||
        __Cases__[1].__Pattern__ == NULL || __Cases__[1].__Body__ == NULL)
    {
        return 0;
    }

    /* match tmp with ... */
    __Scrutinee_Lvalue__ = __Parser_New_Base_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base_Temporary__, __Empty__, __Temporary__, __Mark__);
    __Scrutinee__ = __Parser_New_Expression__(__Parser_State__, __Ast_Expression_Atom__, __Mark__);
    __Match__ = __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Match__, __Mark__);
    if (__Scrutinee_Lvalue__ == NULL || __Scrutinee__ == NULL || __Match__ == NULL)
    {
        return 0;
    }
    __Scrutinee__->__As__.__Atom__.__Kind__ = __Ast_Atom_Lvalue__;
    __Scrutinee__->__As__.__Atom__.__As__.__Lvalue__ = __Scrutinee_Lvalue__;
    __Match__->__As__.__Match__.__Value__ = __Scrutinee__;
    __Match__->__As__.__Match__.__Case_Count__ = 2U;
    __Match__->__As__.__Match__.__Cases__ =
        (__Ast_Match_Case__ *)__Ast_Copy_Array__(__Parser_State__->__Ast__,
                                                 __Cases__,
                                                 2U,
                                                 sizeof(__Cases__[0]),
                                                 alignof(__Ast_Match_Case__));
    if (__Match__->__As__.__Match__.__Cases__ == NULL)
    {
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Match__);
}
