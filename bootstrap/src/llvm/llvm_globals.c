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
    if (!global->__Declaration__->__External__)
    {
        __LLVM_Fail__("Stage0 lowers only external module-level data");
        return 1;
    }
    value_type = __Type_Unwrap_Mutable__(global->__Declaration__->__Type__);
    llvm_type = value_type == NULL ? NULL : __LLVM_Type__(emitter, value_type);
    if (value_type == NULL || llvm_type == NULL)
    {
        __LLVM_Fail__("external data has no canonical LLVM value type");
        return 1;
    }
    symbol = __LLVM_Declare_External_Global__(
        emitter, global->__Declaration__->__Link_Name__, llvm_type);
    if (symbol == NULL)
        return 1;
    out_place->address = symbol;
    out_place->llvm_type = llvm_type;
    out_place->type = value_type;
    return 1;
}
