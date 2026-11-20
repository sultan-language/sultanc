/* Owns Stage0 LLVM lowering of module-level external data.
 *
 * Canonical behavior: compiler/backend/provider/llvm/module.sn declares an
 * MSIR global with an external name as an LLVM global with that exact symbol,
 * the declared value type and no initializer; loads and stores go through it
 * as an ordinary place. Executable-owned startup symbols reuse such a
 * declaration (compiler/backend/provider/llvm/entry.sn). */

#include "llvm/llvm_globals.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_types.h"
#include "kernel/name/name.h"
#include "kernel/type/type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Checks that an LLVM value carries exactly the requested name. */
static int __LLVM_Global_Name_Matches__(LLVMValueRef value, const char *name, size_t length)
{
    /* Stores the actual name length. */
    size_t actual_length = 0U;
    /* References the actual name. */
    const char *actual = LLVMGetValueName2(value, &actual_length);

    return actual != NULL && actual_length == length &&
           (length == 0U || memcmp(actual, name, length) == 0);
}

LLVMValueRef __LLVM_Declare_External_Global__(__LLVM_Emitter__ *emitter,
                                              __Text_Slice__ link_name,
                                              LLVMTypeRef value_type)
{
    /* Owns the NUL-terminated linker name. */
    char *name;
    /* Stores the global. */
    LLVMValueRef global;

    if (emitter == NULL || emitter->module == NULL || value_type == NULL ||
        link_name.__Data__ == NULL || link_name.__Length__ == 0U ||
        memchr(link_name.__Data__, '\0', link_name.__Length__) != NULL)
    {
        __LLVM_Fail__("external data requires a nonempty linker name without NUL");
        return NULL;
    }
    name = (char *)malloc(link_name.__Length__ + 1U);
    if (name == NULL)
    {
        __LLVM_Fail__("out of memory while declaring external data");
        return NULL;
    }
    memcpy(name, link_name.__Data__, link_name.__Length__);
    name[link_name.__Length__] = '\0';

    global = LLVMGetNamedGlobal(emitter->module, name);
    if (global != NULL)
    {
        free(name);
        /* A conflicting declaration must not be turned into a typed load of
         * the wrong LLVM type. */
        if (LLVMGlobalGetValueType(global) != value_type)
        {
            __LLVM_Fail__("external data symbol is declared with conflicting types");
            return NULL;
        }
        return global;
    }
    if (LLVMGetNamedFunction(emitter->module, name) != NULL)
    {
        free(name);
        __LLVM_Fail__("external data symbol conflicts with a function symbol");
        return NULL;
    }
    global = LLVMAddGlobal(emitter->module, value_type, name);
    if (global == NULL || !__LLVM_Global_Name_Matches__(global, name, link_name.__Length__))
    {
        free(name);
        __LLVM_Fail__("LLVM could not declare external data with its exact linker name");
        return NULL;
    }
    free(name);
    /* An imported linker symbol: external linkage, no initializer. */
    LLVMSetLinkage(global, LLVMExternalLinkage);
    return global;
}

/* Builds the LLVM constant of a folded module constant of TYPE. */
static LLVMValueRef __LLVM_Constant_Value__(__LLVM_Emitter__ *emitter,
                                           const __Ast_Constant__ *constant,
                                           __Ast_Type__ *type,
                                           int writable_data);

/* Builds a {data, length, capacity} header whose data is a private array of
 * ELEMENT_TYPE values; capacity 0 marks storage no drop releases and that an
 * append copies before growing (the text-literal convention). */
static LLVMValueRef __LLVM_Constant_Sequence__(__LLVM_Emitter__ *emitter,
                                              LLVMTypeRef header_type,
                                              LLVMTypeRef element_type,
                                              LLVMValueRef *elements,
                                              size_t count,
                                              int writable_data)
{
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMValueRef fields[3];
    if (count == 0U)
    {
        fields[0] = LLVMConstNull(LLVMPointerTypeInContext(emitter->context, 0U));
    }
    else
    {
        LLVMTypeRef array_type = LLVMArrayType2(element_type, (uint64_t)count);
        LLVMValueRef data = LLVMAddGlobal(emitter->module, array_type, "sultanc.global.data");
        if (data == NULL)
        {
            __LLVM_Fail__("could not materialize module constant data");
            return NULL;
        }
        LLVMSetInitializer(data, LLVMConstArray2(element_type, elements, (uint64_t)count));
        LLVMSetLinkage(data, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(data, writable_data ? 0 : 1);
        fields[0] = data;
    }
    fields[1] = LLVMConstInt(i64, (unsigned long long)count, 0);
    fields[2] = LLVMConstInt(i64, 0U, 0);
    if (LLVMGetTypeKind(header_type) != LLVMStructTypeKind ||
        LLVMCountStructElementTypes(header_type) != 3U)
    {
        __LLVM_Fail__("module constant sequence disagrees with the canonical header layout");
        return NULL;
    }
    if (LLVMIsLiteralStruct(header_type))
        return LLVMConstStructInContext(emitter->context, fields, 3U, 0);
    return LLVMConstNamedStruct(header_type, fields, 3U);
}

static LLVMValueRef __LLVM_Constant_Value__(__LLVM_Emitter__ *emitter,
                                           const __Ast_Constant__ *constant,
                                           __Ast_Type__ *type,
                                           int writable_data)
{
    __Resolved_Type__ resolved;
    LLVMTypeRef llvm_type;
    if (constant == NULL || type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved) ||
        (llvm_type = __LLVM_Type__(emitter, type)) == NULL)
    {
        __LLVM_Fail__("module constant has no canonical LLVM type");
        return NULL;
    }
    switch (constant->__Kind__)
    {
        case __Ast_Constant_Integer__:
        case __Ast_Constant_Boolean__:
        case __Ast_Constant_Character__:
        {
            unsigned bits;
            unsigned long long raw = (unsigned long long)constant->__Integer__;
            if (LLVMGetTypeKind(llvm_type) != LLVMIntegerTypeKind)
            {
                __LLVM_Fail__("module constant integer has a non-integer LLVM type");
                return NULL;
            }
            bits = LLVMGetIntTypeWidth(llvm_type);
            if (bits < 64U)
                raw &= (1ULL << bits) - 1ULL;
            return LLVMConstInt(llvm_type, raw, 0);
        }
        case __Ast_Constant_Float__:
        {
            char *spelling = (char *)malloc(constant->__Text__.__Length__ + 2U);
            LLVMValueRef value;
            size_t offset = 0U;
            if (spelling == NULL)
            {
                __LLVM_Fail__("out of memory while lowering a float constant");
                return NULL;
            }
            if (constant->__Negative_Float__)
                spelling[offset++] = '-';
            memcpy(spelling + offset, constant->__Text__.__Data__, constant->__Text__.__Length__);
            offset += constant->__Text__.__Length__;
            spelling[offset] = '\0';
            if (constant->__Float_Step_Count__ == 0U)
            {
                value = LLVMConstRealOfStringAndSize(llvm_type, spelling, (unsigned)offset);
            }
            else
            {
                /* Parsed at the first width, then converted through the
                 * others (constant-folded by the builder). */
                size_t step;
                LLVMTypeRef step_type = constant->__Float_Steps__[0] == 32U
                                            ? LLVMFloatTypeInContext(emitter->context)
                                            : LLVMDoubleTypeInContext(emitter->context);
                value = LLVMConstRealOfStringAndSize(step_type, spelling, (unsigned)offset);
                for (step = 1U; step < constant->__Float_Step_Count__; ++step)
                {
                    LLVMTypeRef next = constant->__Float_Steps__[step] == 32U
                                           ? LLVMFloatTypeInContext(emitter->context)
                                           : LLVMDoubleTypeInContext(emitter->context);
                    value = constant->__Float_Steps__[step] < constant->__Float_Steps__[step - 1U]
                                ? LLVMBuildFPTrunc(emitter->builder, value, next, "constant.fptrunc")
                                : LLVMBuildFPExt(emitter->builder, value, next, "constant.fpext");
                }
                if (!LLVMIsConstant(value))
                {
                    free(spelling);
                    __LLVM_Fail__("float constant conversion did not fold");
                    return NULL;
                }
            }
            free(spelling);
            return value;
        }
        case __Ast_Constant_Text__:
        {
            LLVMTypeRef byte = LLVMIntTypeInContext(emitter->context, 8U);
            LLVMValueRef *bytes = NULL;
            LLVMValueRef header;
            size_t index;
            if (constant->__Text__.__Length__ != 0U)
            {
                bytes = (LLVMValueRef *)malloc(constant->__Text__.__Length__ * sizeof(*bytes));
                if (bytes == NULL)
                {
                    __LLVM_Fail__("out of memory while lowering a text constant");
                    return NULL;
                }
                for (index = 0U; index < constant->__Text__.__Length__; ++index)
                    bytes[index] = LLVMConstInt(
                        byte, (unsigned char)constant->__Text__.__Data__[index], 0);
            }
            header = __LLVM_Constant_Sequence__(emitter, llvm_type, byte, bytes,
                                                constant->__Text__.__Length__, writable_data);
            free(bytes);
            return header;
        }
        case __Ast_Constant_Vector__:
        {
            LLVMTypeRef element_type;
            LLVMValueRef *elements = NULL;
            LLVMValueRef header;
            size_t index;
            if (resolved.__Kind__ != __Resolved_Type_Vector__ || resolved.__Inner__ == NULL ||
                (element_type = __LLVM_Type__(emitter, resolved.__Inner__)) == NULL)
            {
                __LLVM_Fail__("module vector constant has no canonical element type");
                return NULL;
            }
            if (constant->__Element_Count__ != 0U)
            {
                elements = (LLVMValueRef *)malloc(constant->__Element_Count__ * sizeof(*elements));
                if (elements == NULL)
                {
                    __LLVM_Fail__("out of memory while lowering a vector constant");
                    return NULL;
                }
                for (index = 0U; index < constant->__Element_Count__; ++index)
                {
                    elements[index] = __LLVM_Constant_Value__(
                        emitter, &constant->__Elements__[index], resolved.__Inner__, 0);
                    if (elements[index] == NULL)
                    {
                        free(elements);
                        return NULL;
                    }
                }
            }
            header = __LLVM_Constant_Sequence__(emitter, llvm_type, element_type, elements,
                                                constant->__Element_Count__, writable_data);
            free(elements);
            return header;
        }
    }
    __LLVM_Fail__("module constant kind is outside Stage0 lowering");
    return NULL;
}

/* Defines (once) the LLVM storage of a module constant or variable. */
static LLVMValueRef __LLVM_Define_Value_Global__(__LLVM_Emitter__ *emitter,
                                                __Semantic_Global_Entry__ *global,
                                                __Ast_Type__ *value_type,
                                                LLVMTypeRef llvm_type)
{
    char name[64];
    size_t index = 0U;
    __Semantic_Global_Entry__ *first =
        (__Semantic_Global_Entry__ *)__Vector_At__(&emitter->semantic->__Globals__, 0U);
    LLVMValueRef symbol;
    LLVMValueRef initializer;
    int mutable_global = global->__Declaration__->__Type__ != NULL &&
                         global->__Declaration__->__Type__->__Kind__ == __Ast_Type_Mutable__;
    if (first != NULL && global >= first)
        index = (size_t)(global - first);
    snprintf(name, sizeof(name), "sultanc.global.%zu", index);
    symbol = LLVMGetNamedGlobal(emitter->module, name);
    if (symbol != NULL)
        return symbol;
    if (global->__Declaration__->__Constant__ == NULL)
    {
        __LLVM_Fail__("module value was not folded by the semantic pass");
        return NULL;
    }
    initializer = __LLVM_Constant_Value__(emitter, global->__Declaration__->__Constant__,
                                          value_type, 1);
    if (initializer == NULL)
        return NULL;
    symbol = LLVMAddGlobal(emitter->module, llvm_type, name);
    if (symbol == NULL)
    {
        __LLVM_Fail__("could not define module value storage");
        return NULL;
    }
    LLVMSetInitializer(symbol, initializer);
    LLVMSetLinkage(symbol, LLVMInternalLinkage);
    LLVMSetGlobalConstant(symbol, mutable_global ? 0 : 1);
    return symbol;
}

int __LLVM_Resolve_Global_Place__(__LLVM_Emitter__ *emitter,
                                  __Ast_Lvalue__ *lvalue,
                                  __LLVM_Place__ *out_place)
{
    /* References the semantic declaration. */
    __Semantic_Global_Entry__ *global = NULL;
    /* References the value type. */
    __Ast_Type__ *value_type;
    /* Stores the LLVM value type. */
    LLVMTypeRef llvm_type;
    /* Stores the LLVM global. */
    LLVMValueRef symbol;

    if (out_place != NULL)
        memset(out_place, 0, sizeof(*out_place));
    if (emitter == NULL || out_place == NULL || lvalue == NULL ||
        lvalue->__Kind__ != __Ast_Lvalue_Base__ ||
        lvalue->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        emitter->current_function == NULL)
        return 0;
    if (__Name_Resolve_Global__(emitter->semantic,
                                emitter->current_function->__Unit__,
                                lvalue->__As__.__Base__.__As__.__Identifier__,
                                &global) != __Name_Lookup_Found__ ||
        global == NULL || global->__Declaration__ == NULL)
        return 0;
    value_type = __Type_Unwrap_Mutable__(global->__Declaration__->__Type__);
    llvm_type = value_type == NULL ? NULL : __LLVM_Type__(emitter, value_type);
    if (value_type == NULL || llvm_type == NULL)
    {
        __LLVM_Fail__("module-level data has no canonical LLVM value type");
        return 1;
    }
    if (global->__Declaration__->__External__)
        symbol = __LLVM_Declare_External_Global__(
            emitter, global->__Declaration__->__Link_Name__, llvm_type);
    else
        symbol = __LLVM_Define_Value_Global__(emitter, global, value_type, llvm_type);
    if (symbol == NULL)
        return 1;
    out_place->address = symbol;
    out_place->llvm_type = llvm_type;
    out_place->type = value_type;
    return 1;
}
