/* Owns Sultan type to LLVM type conversion for Stage0. */

#include "llvm/llvm_types.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_functions.h"
#include "llvm/llvm_values.h"
#include "frontend/identifier_identity.h"
#include "kernel/layout/layout.h"
#include "kernel/name/name.h"
#include "kernel/type/type.h"
#include <limits.h>
#include <stdlib.h>

/* Stores the LLVM boolean type. */
__Ast_Type__ __LLVM_Boolean_Type__ = {.__Kind__ = __Ast_Type_Boolean__};
/* Stores the LLVM integer type. */
__Ast_Type__ __LLVM_Integer_Type__ = {.__Kind__ = __Ast_Type_Integer__};
/* Stores the LLVM string type. */
__Ast_Type__ __LLVM_String_Type__ = {.__Kind__ = __Ast_Type_String__};
/* Stores the LLVM void type. */
__Ast_Type__ __LLVM_Void_Type__ = {.__Kind__ = __Ast_Type_Void__};
/* Stores the LLVM result text integer type. */
__Ast_Type__ __LLVM_Result_Text_Integer_Type__ = {
    .__Kind__ = __Ast_Type_Result__,
    .__As__.__Result__ = {&__LLVM_String_Type__, &__LLVM_Integer_Type__}};
/* Stores the LLVM result integer integer type. */
__Ast_Type__ __LLVM_Result_Integer_Integer_Type__ = {
    .__Kind__ = __Ast_Type_Result__,
    .__As__.__Result__ = {&__LLVM_Integer_Type__, &__LLVM_Integer_Type__}};
/* Stores the LLVM u 8 type. */
__Ast_Type__ __LLVM_U8_Type__ = {.__Kind__ = __Ast_Type_Machine__,
                                        .__As__.__Machine__ = __Machine_U8__};

/* Resolves the LLVM integer. */
int __LLVM_Resolve_Integer__(__LLVM_Emitter__ *emitter,
                                    __Ast_Type__ *type,
                                    unsigned *bits,
                                    int *is_signed)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    if (type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved) ||
        (resolved.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         resolved.__Kind__ != __Resolved_Type_Unsigned_Integer__))
    {
        return 0;
    }
    *bits = resolved.__Bits__;
    *is_signed = resolved.__Kind__ == __Resolved_Type_Signed_Integer__;
    return *bits != 0U;
}

/* Resolves the LLVM struct field. */
int __LLVM_Resolve_Struct_Field__(__LLVM_Emitter__ *emitter,
                                         __Ast_Type__ *parent_type,
                                         __Text_Slice__ field_name,
                                         size_t *out_index,
                                         size_t *out_offset,
                                         __Ast_Type__ **out_type)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* References the declaration. */
    __Ast_Type_Declaration__ *declaration;
    /* Tracks the index. */
    size_t index;
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* References the canonical type. */
    __Ast_Type__ *canonical_type = NULL;

    if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Struct__ || resolved.__Named__ == NULL ||
        resolved.__Named__->__Declaration__ == NULL)
    {
        return 0;
    }
    declaration = resolved.__Named__->__Declaration__;
    if (declaration->__Kind__ != __Ast_Type_Decl_Struct__ ||
        !__Layout_Struct_Field__(
            emitter->semantic, resolved.__Named__, field_name, &canonical_offset, &canonical_type))
    {
        return 0;
    }
    for (index = 0U; index < declaration->__As__.__Struct__.__Count__; ++index)
    {
        /* References the field. */
        __Ast_Struct_Field__ *field = &declaration->__As__.__Struct__.__Fields__[index];
        if (__Identifier_Identity_Equals__(field->__Name__, field_name))
        {
            if (field->__Slot__.__Type__ != canonical_type)
            {
                return __LLVM_Fail__("canonical struct field Type/layout disagreement");
            }
            if (out_index != NULL)
                *out_index = index;
            if (out_offset != NULL)
                *out_offset = canonical_offset;
            if (out_type != NULL)
                *out_type = canonical_type;
            return 1;
        }
    }
    return 0;
}

/* Checks whether the LLVM enum is payload free. */
int __LLVM_Enum_Is_Payload_Free__(__Semantic_Type_Entry__ *entry)
{
    /* References the declaration. */
    __Ast_Type_Declaration__ *declaration;
    /* Tracks the index. */
    size_t index;
    if (entry == NULL || (declaration = entry->__Declaration__) == NULL ||
        declaration->__Kind__ != __Ast_Type_Decl_Enum__)
        return 0;
    for (index = 0U; index < declaration->__As__.__Enum__.__Count__; ++index)
    {
        if (declaration->__As__.__Enum__.__Constructors__[index].__Payload_Count__ != 0U)
            return 0;
    }
    return 1;
}

/* Returns the LLVM tagged layout. */
int __LLVM_Tagged_Layout__(__LLVM_Emitter__ *emitter,
                                  __Ast_Type__ *type,
                                  size_t *payload_size,
                                  size_t *payload_offset,
                                  size_t *alignment)
{
    /* Stores the tag size. */
    size_t tag_size = 0U;
    /* Stores the size. */
    size_t size = 0U;
    /* Stores the offset. */
    size_t offset = 0U;
    /* Stores the align. */
    size_t align = 1U;
    if (!__Layout_Tagged_Storage__(emitter->semantic, type, &tag_size, &offset, &size, &align))
    {
        return __LLVM_Fail__("L2.5 could not consume canonical tagged layout");
    }
    /* Tagged storage uses an eight-byte tag followed by aligned payload bytes. */
    if (tag_size != 8U || offset != 8U || align > 8U || size > (size_t)UINT_MAX)
    {
        return __LLVM_Fail__(
            "L2.5 canonical tagged layout is not representable by current direct LLVM storage");
    }
    if (payload_size != NULL)
        *payload_size = size;
    if (payload_offset != NULL)
        *payload_offset = offset;
    if (alignment != NULL)
        *alignment = align;
    return 1;
}

/* Returns the LLVM tagged layout for a canonical named enum entry. */
int __LLVM_Tagged_Entry_Layout__(__LLVM_Emitter__ *emitter,
                                 __Semantic_Type_Entry__ *entry,
                                 size_t *payload_size,
                                 size_t *payload_offset,
                                 size_t *alignment)
{
    size_t tag_size = 0U;
    size_t size = 0U;
    size_t offset = 0U;
    size_t align = 1U;
    if (!__Layout_Tagged_Entry_Storage__(
            emitter->semantic, entry, &tag_size, &offset, &size, &align))
    {
        return __LLVM_Fail__("L2.5 could not consume canonical tagged entry layout");
    }
    if (tag_size != 8U || offset != 8U || align > 8U || size > (size_t)UINT_MAX)
    {
        return __LLVM_Fail__(
            "L2.5 canonical tagged entry layout is not representable by current direct LLVM storage");
    }
    if (payload_size != NULL)
        *payload_size = size;
    if (payload_offset != NULL)
        *payload_offset = offset;
    if (alignment != NULL)
        *alignment = align;
    return 1;
}

/* Creates the LLVM tagged literal type. */
static LLVMTypeRef __LLVM_Create_Tagged_Literal_Type__(__LLVM_Emitter__ *emitter,
                                                       __Ast_Type__ *type)
{
    /* Stores the elements. */
    LLVMTypeRef elements[2];
    /* Stores the payload size. */
    size_t payload_size = 0U;
    if (!__LLVM_Tagged_Layout__(emitter, type, &payload_size, NULL, NULL))
    {
        return NULL;
    }
    elements[0] = LLVMIntTypeInContext(emitter->context, 64U);
    elements[1] = LLVMArrayType(LLVMIntTypeInContext(emitter->context, 8U), (unsigned)payload_size);
    return LLVMStructTypeInContext(emitter->context, elements, 2U, 0);
}

/* Returns the LLVM type. */
LLVMTypeRef __LLVM_Type__(__LLVM_Emitter__ *emitter, __Ast_Type__ *type)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    if (type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved))
    {
        __LLVM_Fail__("L2.5 could not resolve Type");
        return NULL;
    }
    if (resolved.__Kind__ == __Resolved_Type_Boolean__)
    {
        return LLVMIntTypeInContext(emitter->context, 1U);
    }
    if (resolved.__Kind__ == __Resolved_Type_Signed_Integer__ ||
        resolved.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return LLVMIntTypeInContext(emitter->context, resolved.__Bits__);
    }
    if (resolved.__Kind__ == __Resolved_Type_Float__)
    {
        if (resolved.__Bits__ == 32U)
            return LLVMFloatTypeInContext(emitter->context);
        if (resolved.__Bits__ == 64U)
            return LLVMDoubleTypeInContext(emitter->context);
        __LLVM_Fail__("LLVM float Type must be 32 or 64 bits");
        return NULL;
    }
    if ((resolved.__Kind__ == __Resolved_Type_Struct__ ||
         resolved.__Kind__ == __Resolved_Type_Enum__) &&
        resolved.__Named__ != NULL)
    {
        /* References the aggregate. */
        __LLVM_Aggregate_Type__ *aggregate =
            __LLVM_Find_Aggregate_Type__(emitter, resolved.__Named__);
        if (aggregate != NULL)
        {
            return aggregate->type;
        }
        __LLVM_Fail__("L2.5 named aggregate Type has no canonical LLVM declaration");
        return NULL;
    }
    if (resolved.__Kind__ == __Resolved_Type_Option__ ||
        resolved.__Kind__ == __Resolved_Type_Result__)
    {
        return __LLVM_Create_Tagged_Literal_Type__(emitter, type);
    }
    if (resolved.__Kind__ == __Resolved_Type_Pointer__ ||
        resolved.__Kind__ == __Resolved_Type_Reference__ ||
        resolved.__Kind__ == __Resolved_Type_Box__)
    {
        /* Stores the inner. */
        LLVMTypeRef inner =
            resolved.__Inner__ != NULL ? __LLVM_Type__(emitter, resolved.__Inner__) : NULL;
        if (inner == NULL)
        {
            __LLVM_Fail__("L2.6 reference/box Type has no canonical inner Type");
            return NULL;
        }
        return LLVMPointerType(inner, 0U);
    }
    if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
        resolved.__Kind__ == __Resolved_Type_String__)
    {
        /* Stores the elements. */
        LLVMTypeRef elements[3];
        /* Stores the element. */
        LLVMTypeRef element = NULL;
        /* Stores the size. */
        size_t size = 0U, alignment = 0U;
        /* Stores the data offset. */
        size_t data_offset, length_offset, capacity_offset;
        if (resolved.__Kind__ == __Resolved_Type_Vector__)
        {
            element =
                resolved.__Inner__ != NULL ? __LLVM_Type__(emitter, resolved.__Inner__) : NULL;
            data_offset = __Layout_Vector_Data_Offset__();
            length_offset = __Layout_Vector_Length_Offset__();
            capacity_offset = __Layout_Vector_Capacity_Offset__();
        }
        else
        {
            element = LLVMIntTypeInContext(emitter->context, 8U);
            data_offset = __Layout_String_Data_Offset__();
            length_offset = __Layout_String_Length_Offset__();
            capacity_offset = __Layout_String_Capacity_Offset__();
        }
        if (element == NULL || !__Layout_Type__(emitter->semantic, type, &size, &alignment) ||
            size != 24U || alignment != 8U || data_offset != 0U || length_offset != 8U ||
            capacity_offset != 16U)
        {
            __LLVM_Fail__("L2.7 sequence Type disagrees with canonical Stage0 layout");
            return NULL;
        }
        elements[0] = LLVMPointerType(element, 0U);
        elements[1] = LLVMIntTypeInContext(emitter->context, 64U);
        elements[2] = LLVMIntTypeInContext(emitter->context, 64U);
        return LLVMStructTypeInContext(emitter->context, elements, 3U, 0);
    }
    if (resolved.__Kind__ == __Resolved_Type_Function__)
    {
        LLVMTypeRef *parameters = NULL;
        LLVMTypeRef output;
        LLVMTypeRef function_type;
        size_t index;
        if (resolved.__Parameter_Count__ != 0U)
        {
            parameters = (LLVMTypeRef *)calloc(resolved.__Parameter_Count__, sizeof(*parameters));
            if (parameters == NULL)
            {
                __LLVM_Fail__("out of memory while lowering function Type");
                return NULL;
            }
        }
        for (index = 0U; index < resolved.__Parameter_Count__; ++index)
        {
            parameters[index] = __LLVM_Type__(emitter, resolved.__Parameters__[index]);
            if (parameters[index] == NULL)
            {
                free(parameters);
                return NULL;
            }
        }
        output = __LLVM_Type__(emitter, resolved.__Output__);
        if (output == NULL)
        {
            free(parameters);
            return NULL;
        }
        function_type = LLVMFunctionType(
            output, parameters, (unsigned)resolved.__Parameter_Count__, 0);
        free(parameters);
        return function_type == NULL ? NULL : LLVMPointerType(function_type, 0U);
    }
    if (resolved.__Kind__ == __Resolved_Type_Void__)
    {
        return LLVMVoidTypeInContext(emitter->context);
    }
    __LLVM_Fail__("Direct LLVM Type is outside the accepted L2.1-L2.7 profile");
    return NULL;
}

/* Returns the LLVM concrete integer type. */
__Ast_Type__ *__LLVM_Concrete_Integer_Type__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Type__ *candidate)
{
    /* Stores the bits. */
    unsigned bits;
    /* Tracks whether the value is signed. */
    int is_signed;

    return candidate != NULL && __LLVM_Resolve_Integer__(emitter, candidate, &bits, &is_signed)
               ? candidate
               : NULL;
}

/* Returns the LLVM expression integer type. */
__Ast_Type__ *__LLVM_Expression_Integer_Type__(__LLVM_Emitter__ *emitter,
                                                      __Ast_Expression__ *expression)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *lvalue;
    /* References the type. */
    __Ast_Type__ *type;

    if (expression == NULL)
    {
        return NULL;
    }
    switch (expression->__Kind__)
    {
        case __Ast_Expression_Atom__:
            if (expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
            {
                return NULL;
            }
            lvalue = expression->__As__.__Atom__.__As__.__Lvalue__;
            return __LLVM_Lvalue_Type__(emitter, lvalue);

        case __Ast_Expression_Conversion__:
            return expression->__As__.__Conversion__.__Target_Type__;

        case __Ast_Expression_Binary__:
            type =
                __LLVM_Expression_Integer_Type__(emitter, expression->__As__.__Binary__.__Left__);
            return type != NULL ? type
                                : __LLVM_Expression_Integer_Type__(
                                      emitter, expression->__As__.__Binary__.__Right__);

        case __Ast_Expression_Call__:
        {
            /* References the callee. */
            __Semantic_Function_Entry__ *callee =
                __LLVM_Resolve_Ordinary_Callee__(emitter, expression);
            if (callee != NULL)
                return callee->__Function__->__Output__.__Type__;
            switch (__LLVM_Builtin_Call_Identity__(emitter, expression))
            {
                case __Name_Builtin_Length__:
                case __Name_Builtin_Host_Architecture__:
                case __Name_Builtin_Host_Platform__:
                case __Name_Builtin_Host_Environment__:
                    return &__LLVM_Integer_Type__;
                default:
                    return NULL;
            }
        }

        case __Ast_Expression_Unary__:
        {
            __Ast_Type__ *operand_type;
            __Resolved_Type__ resolved;
            if (expression->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
            {
                operand_type = __LLVM_Expression_Type__(
                    emitter, expression->__As__.__Unary__.__Operand__);
                if (operand_type != NULL &&
                    __Type_Resolve__(emitter->semantic, operand_type, &resolved) &&
                    (resolved.__Kind__ == __Resolved_Type_Reference__ ||
                     resolved.__Kind__ == __Resolved_Type_Pointer__ ||
                     resolved.__Kind__ == __Resolved_Type_Box__))
                {
                    return resolved.__Inner__;
                }
                return NULL;
            }
            return __LLVM_Expression_Integer_Type__(
                emitter, expression->__As__.__Unary__.__Operand__);
        }
    }
    return NULL;
}

/* Returns the LLVM expression type. */
__Ast_Type__ *__LLVM_Expression_Type__(__LLVM_Emitter__ *emitter,
                                              __Ast_Expression__ *expression)
{
    if (expression == NULL)
    {
        return NULL;
    }
    if (expression->__Contextual_Type__ != NULL)
    {
        return expression->__Contextual_Type__;
    }
    switch (expression->__Kind__)
    {
        case __Ast_Expression_Atom__:
            if (expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
            {
                return __LLVM_Lvalue_Type__(emitter, expression->__As__.__Atom__.__As__.__Lvalue__);
            }
            if (expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Literal__ &&
                expression->__As__.__Atom__.__As__.__Literal__ != NULL &&
                expression->__As__.__Atom__.__As__.__Literal__->__Kind__ == __Ast_Literal_String__)
            {
                return &__LLVM_String_Type__;
            }
            return NULL;
        case __Ast_Expression_Conversion__:
            return expression->__As__.__Conversion__.__Target_Type__;
        case __Ast_Expression_Call__:
        {
            /* References the callee. */
            __Semantic_Function_Entry__ *callee =
                __LLVM_Resolve_Ordinary_Callee__(emitter, expression);
            /* Stores the builtin. */
            __Name_Builtin_Function__ builtin;
            if (callee != NULL)
                return callee->__Function__->__Output__.__Type__;
            builtin = __LLVM_Builtin_Call_Identity__(emitter, expression);
            switch (builtin)
            {
                case __Name_Builtin_Length__:
                case __Name_Builtin_Host_Architecture__:
                case __Name_Builtin_Host_Platform__:
                case __Name_Builtin_Host_Environment__:
                    return &__LLVM_Integer_Type__;
                case __Name_Builtin_Append__:
                case __Name_Builtin_Swap__:
                    return &__LLVM_Void_Type__;
                default:
                    return expression->__Contextual_Type__;
            }
        }
        case __Ast_Expression_Binary__:
            return __LLVM_Expression_Integer_Type__(emitter, expression);
        case __Ast_Expression_Unary__:
        {
            /* Stores the operation. */
            __Ast_Unary_Operation__ operation = expression->__As__.__Unary__.__Operation__;
            /* References the operand type. */
            __Ast_Type__ *operand_type;
            /* Stores the resolved. */
            __Resolved_Type__ resolved;
            if (operation == __Unary_Not__)
                return &__LLVM_Boolean_Type__;
            if (operation == __Unary_Address__ || operation == __Unary_Address_Mutable__)
                return expression->__Contextual_Type__;
            operand_type =
                __LLVM_Expression_Type__(emitter, expression->__As__.__Unary__.__Operand__);
            if (operation == __Unary_Dereference__ && operand_type != NULL &&
                __Type_Resolve__(emitter->semantic, operand_type, &resolved) &&
                (resolved.__Kind__ == __Resolved_Type_Reference__ ||
                 resolved.__Kind__ == __Resolved_Type_Pointer__ ||
                 resolved.__Kind__ == __Resolved_Type_Box__))
                return resolved.__Inner__;
            return operand_type;
        }
    }
    return NULL;
}
