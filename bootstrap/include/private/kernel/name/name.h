/* Declares canonical name resolution. */

#ifndef SULTANC__KERNEL_NAME_NAME_H__
#define SULTANC__KERNEL_NAME_NAME_H__

#include "semantic/context.h"

/* Defines the name lookup status values. */
typedef enum
{
    /* Represents the name lookup missing value. */
    __Name_Lookup_Missing__,
    /* Represents the name lookup found value. */
    __Name_Lookup_Found__,
    /* Represents the name lookup private value. */
    __Name_Lookup_Private__,
    /* Represents the name lookup ambiguous value. */
    __Name_Lookup_Ambiguous__
} __Name_Lookup_Status__;

/* Canonical intrinsic function identities used by Stage0. */
typedef enum
{
    /* Represents the name builtin none value. */
    __Name_Builtin_None__,
    /* Represents the name builtin length value. */
    __Name_Builtin_Length__,
    /* Represents the name builtin append value. */
    __Name_Builtin_Append__,
    /* Represents the name builtin swap value. */
    __Name_Builtin_Swap__,
    /* Represents the selected-target architecture query. */
    __Name_Builtin_Host_Architecture__,
    /* Represents the selected-target platform query. */
    __Name_Builtin_Host_Platform__,
    /* Represents the selected-target environment query. */
    __Name_Builtin_Host_Environment__,
    /* Canonical scalar bit-reinterpretation primitives (never numeric casts). */
    __Name_Builtin_F64_Bits__,
    __Name_Builtin_F64_From_Bits__,
    __Name_Builtin_F32_Bits__,
    __Name_Builtin_F32_From_Bits__,
    __Name_Builtin_Release__,
    /* Canonical بدائي_نداء_نواة_خام: one Linux 64-bit kernel entry instruction
     * (compiler/frontend/names/primitive_registry.sn). Stage0 only emits it;
     * the syscall number and operand meaning are owned by the STD source. */
    __Name_Builtin_Raw_Kernel_Call__,
    /* Vector primitives (docs/en/reference/ownership.md): capacity,
     * reserve_exact, pop and truncate. */
    __Name_Builtin_Capacity__,
    __Name_Builtin_Reserve_Exact__,
    __Name_Builtin_Pop__,
    __Name_Builtin_Truncate__,
    /* ألحق_من / extend_from(vector, &source, start, count): appends the range
     * [start, start + count) of a vector of the same integer element Type,
     * or of a text when the element Type is ط8 (Rust's
     * Vec::extend_from_slice). A range past the source's end traps. The
     * buffer grows once, then the range is copied as one block. */
    __Name_Builtin_Extend_From__,
    /* Wrapping arithmetic (docs/en/reference/type-system.md): reached through private
     * aliases of their بدائي_ spellings, as std.math.wrapping does. */
    __Name_Builtin_Wrapping_Add__,
    __Name_Builtin_Wrapping_Subtract__,
    __Name_Builtin_Wrapping_Multiply__,
    __Name_Builtin_Wrapping_Shift_Left__,
    __Name_Builtin_Wrapping_Shift_Right__,
    /* Canonical بدائي_نداء_عنوان_سي (compiler/runtime/primitive_identity.sn):
     * a call of the C function at an address with the target's C calling
     * convention. Arguments: the result witness (u64 or f64, the result
     * Type), the address, eight u64 words and eight f64 words. Only the
     * interpreter's foreign-call bridge uses it. */
    __Name_Builtin_C_Address_Call__,
    /* Thread support (compiler/runtime/primitive_identity.sn, docs/en/reference/
     * concurrency.md): atomic operations on the 32- or 64-bit integer (or
     * عدد / طبيعي) a reference refers to. The trailing arguments are memory
     * ordering codes (0 Relaxed .. 4 SeqCst); like the self-hosted LLVM
     * provider, Stage0 lowers every ordering at sequentially consistent
     * strength, which is always sound. */
    /* بدائي_ذري_حمل(&cell, order): the value the cell holds. */
    __Name_Builtin_Atomic_Load__,
    /* بدائي_ذري_خزن(&cell, value, order): stores value. */
    __Name_Builtin_Atomic_Store__,
    /* بدائي_ذري_تبديل(&cell, value, order): stores value, answers the old one. */
    __Name_Builtin_Atomic_Swap__,
    /* بدائي_ذري_قارن_وبدل(&cell, expected, new, success, failure): stores new
     * when the cell holds expected; answers the value the cell held. */
    __Name_Builtin_Atomic_Compare_Swap__,
    /* بدائي_ذري_جلب_جمع / _و / _أو / _أو_حصري(&cell, value, order): the
     * read-modify-write (wrapping add, and, or, xor); answers the old value. */
    __Name_Builtin_Atomic_Fetch_Add__,
    __Name_Builtin_Atomic_Fetch_And__,
    __Name_Builtin_Atomic_Fetch_Or__,
    __Name_Builtin_Atomic_Fetch_Xor__,
    /* بدائي_سياج_ذاكرة(order): a memory fence. */
    __Name_Builtin_Memory_Fence__,
    /* Flat machine memory of a 64-bit native process: بدائي_اقرأ_ذاكرة_الآلة
     * (address) loads and بدائي_اكتب_ذاكرة_الآلة(address, word) stores one
     * 8-byte-aligned u64 word at a u64 address. */
    __Name_Builtin_Machine_Memory_Read__,
    __Name_Builtin_Machine_Memory_Write__,
    /* WebAssembly linear memory (wasm32 only; std's wasm allocation provider):
     * every operand is a u32 offset, length or word into the module's one
     * linear memory. Grow(pages) answers the previous page count (u32 max on
     * failure); read(address) loads and write(address, word) stores one
     * 4-byte-aligned u32; copy(destination, source, length) copies bytes;
     * heap base() is the linker-defined __heap_base address. */
    __Name_Builtin_Linear_Memory_Grow__,
    __Name_Builtin_Linear_Memory_Read__,
    __Name_Builtin_Linear_Memory_Write__,
    __Name_Builtin_Linear_Memory_Copy__,
    __Name_Builtin_Linear_Heap_Base__,
    /* IEEE 754 operations on the float Type (f32 or f64) of the first
     * operand; every operand and the result have that Type
     * (compiler/runtime/primitive_identity.sn, عملية_عائمة_بدائية): sqrt, abs,
     * floor, ceil, trunc, round-ties-even, minimum, maximum (IEEE 754-2019:
     * NaN wins, -0 < +0), copysign and fused multiply-add. */
    __Name_Builtin_Float_Sqrt__,
    __Name_Builtin_Float_Abs__,
    __Name_Builtin_Float_Floor__,
    __Name_Builtin_Float_Ceil__,
    __Name_Builtin_Float_Trunc__,
    __Name_Builtin_Float_Round_Even__,
    __Name_Builtin_Float_Minimum__,
    __Name_Builtin_Float_Maximum__,
    __Name_Builtin_Float_Copy_Sign__,
    __Name_Builtin_Float_Fused_Multiply_Add__
} __Name_Builtin_Function__;

/* Finds the name builtin function. */
__Name_Builtin_Function__ __Name_Find_Builtin_Function__(__Text_Slice__ __Name__);

/* Checks whether the name builtin is direct source. */
int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__);

/* Resolves the name module alias name. */
int __Name_Resolve_Module_Alias_Name__(const __Program_Unit__ *__Unit__,
                                       __Text_Slice__ __Name__,
                                       __Text_Slice__ *__Out_Name__);

/* Resolves the name type. */
__Name_Lookup_Status__ __Name_Resolve_Type__(__Semantic_Context__ *__Context__,
                                             const __Program_Unit__ *__From_Unit__,
                                             __Text_Slice__ __Name__,
                                             __Semantic_Type_Entry__ **__Out_Entry__);

/* Resolves the name function. */
__Name_Lookup_Status__ __Name_Resolve_Function__(__Semantic_Context__ *__Context__,
                                                 const __Program_Unit__ *__From_Unit__,
                                                 __Text_Slice__ __Name__,
                                                 __Semantic_Function_Entry__ **__Out_Entry__);

/* Resolves one module-level value (canonical قيمة_عليا) visible from a unit:
 * the unit's own declarations first, then public declarations of directly
 * imported units, with the same privacy and ambiguity rules as functions. */
__Name_Lookup_Status__ __Name_Resolve_Global__(__Semantic_Context__ *__Context__,
                                               const __Program_Unit__ *__From_Unit__,
                                               __Text_Slice__ __Name__,
                                               __Semantic_Global_Entry__ **__Out_Entry__);

/* Finds the name error ID. */
__Error_Id__ __Name_Lookup_Error_Id__(__Name_Lookup_Status__ __Status__);

/* Context-scoped accessors that delegate to canonical name resolution. */
__Semantic_Type_Entry__ *__Name_Find_Type__(__Semantic_Context__ *__Context__,
                                            __Text_Slice__ __Name__);

/* Finds the name function. */
__Semantic_Function_Entry__ *__Name_Find_Function__(__Semantic_Context__ *__Context__,
                                                    __Text_Slice__ __Name__);

/* Finds the name enum constructor. */
int __Name_Find_Enum_Constructor__(__Semantic_Type_Entry__ *__Type__,
                                   __Text_Slice__ __Constructor_Name__,
                                   size_t *__Out_Index__,
                                   __Ast_Enum_Constructor__ **__Out_Constructor__);

/* Resolves the name enum constructor lvalue. */
int __Name_Resolve_Enum_Constructor_Lvalue__(__Semantic_Context__ *__Context__,
                                             const __Ast_Lvalue__ *__Lvalue__,
                                             __Semantic_Type_Entry__ **__Out_Type__,
                                             size_t *__Out_Index__,
                                             __Ast_Enum_Constructor__ **__Out_Constructor__);

#endif
