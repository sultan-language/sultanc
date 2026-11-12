/* Resolves built-in function identities. */

#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

/* Expands the builtin name macro. */
#define SULTANC__BUILTIN_NAME__(literal_) (__Text_Slice__){(literal_), sizeof(literal_) - 1U}

/* Finds the name builtin function. */
__Name_Builtin_Function__ __Name_Find_Builtin_Function__(__Text_Slice__ __Name__)
{
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("length")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("الطول")))
    {
        return __Name_Builtin_Length__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("append")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("ألحق")))
    {
        return __Name_Builtin_Append__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("swap")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدّل")))
    {
        return __Name_Builtin_Swap__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_معمارية_المضيف")))
    {
        return __Name_Builtin_Host_Architecture__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_منصة_المضيف")))
    {
        return __Name_Builtin_Host_Platform__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بيئة_المضيف")))
    {
        return __Name_Builtin_Host_Environment__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("حرر")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("release")))
        return __Name_Builtin_Release__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نداء_نواة_خام")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("raw_kernel_call")))
        return __Name_Builtin_Raw_Kernel_Call__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بتات_ع64")))
        return __Name_Builtin_F64_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ع64_من_بتات")))
        return __Name_Builtin_F64_From_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بتات_ع32")))
        return __Name_Builtin_F32_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ع32_من_بتات")))
        return __Name_Builtin_F32_From_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("سعة")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("capacity")))
        return __Name_Builtin_Capacity__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("احجز_سعة")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("reserve_exact")))
        return __Name_Builtin_Reserve_Exact__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("أزل_الأخير")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("pop")))
        return __Name_Builtin_Pop__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("ألحق_من")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("extend_from")))
        return __Name_Builtin_Extend_From__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("اقتطع")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("truncate")))
        return __Name_Builtin_Truncate__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_جمع_ملتف")))
        return __Name_Builtin_Wrapping_Add__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_طرح_ملتف")))
        return __Name_Builtin_Wrapping_Subtract__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ضرب_ملتف")))
        return __Name_Builtin_Wrapping_Multiply__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_إزاحة_يسرى_ملتفة")))
        return __Name_Builtin_Wrapping_Shift_Left__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_إزاحة_يمنى_ملتفة")))
        return __Name_Builtin_Wrapping_Shift_Right__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نداء_عنوان_سي")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("raw_c_call")))
        return __Name_Builtin_C_Address_Call__;
    /* Atomics and the fence (reached through private aliases, as std.sync.atomic does). */
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_حمل")))
        return __Name_Builtin_Atomic_Load__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_خزن")))
        return __Name_Builtin_Atomic_Store__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_تبديل")))
        return __Name_Builtin_Atomic_Swap__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_قارن_وبدل")))
        return __Name_Builtin_Atomic_Compare_Swap__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_جلب_جمع")))
        return __Name_Builtin_Atomic_Fetch_Add__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_جلب_و")))
        return __Name_Builtin_Atomic_Fetch_And__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_جلب_أو")))
        return __Name_Builtin_Atomic_Fetch_Or__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ذري_جلب_أو_حصري")))
        return __Name_Builtin_Atomic_Fetch_Xor__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_سياج_ذاكرة")))
        return __Name_Builtin_Memory_Fence__;
    /* Flat machine memory (std.os.allocation's native provider). */
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_اقرأ_ذاكرة_الآلة")))
        return __Name_Builtin_Machine_Memory_Read__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_اكتب_ذاكرة_الآلة")))
        return __Name_Builtin_Machine_Memory_Write__;
    /* WebAssembly linear memory (std.os.allocation's wasm32 provider). */
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نمو_الذاكرة_الخطية")))
        return __Name_Builtin_Linear_Memory_Grow__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_اقرأ_الذاكرة_الخطية")))
        return __Name_Builtin_Linear_Memory_Read__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_اكتب_الذاكرة_الخطية")))
        return __Name_Builtin_Linear_Memory_Write__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_انسخ_الذاكرة_الخطية")))
        return __Name_Builtin_Linear_Memory_Copy__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قاعدة_الكومة_الخطية")))
        return __Name_Builtin_Linear_Heap_Base__;
    /* IEEE float operations (std.math.float). */
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_جذر_تربيعي_عائم")))
        return __Name_Builtin_Float_Sqrt__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قيمة_مطلقة_عائمة")))
        return __Name_Builtin_Float_Abs__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_أرضية_عائمة")))
        return __Name_Builtin_Float_Floor__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_سقف_عائم")))
        return __Name_Builtin_Float_Ceil__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بتر_عائم")))
        return __Name_Builtin_Float_Trunc__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_تقريب_زوجي_عائم")))
        return __Name_Builtin_Float_Round_Even__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_أدنى_عائم")))
        return __Name_Builtin_Float_Minimum__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_أقصى_عائم")))
        return __Name_Builtin_Float_Maximum__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نسخ_إشارة_عائمة")))
        return __Name_Builtin_Float_Copy_Sign__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ضرب_جمع_مدمج_عائم")))
        return __Name_Builtin_Float_Fused_Multiply_Add__;
    return __Name_Builtin_None__;
}

/* Checks whether the name builtin is direct source. */
int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__)
{
    return __Builtin__ == __Name_Builtin_Length__ || __Builtin__ == __Name_Builtin_Append__ ||
           __Builtin__ == __Name_Builtin_Swap__ || __Builtin__ == __Name_Builtin_Release__ ||
           __Builtin__ == __Name_Builtin_Raw_Kernel_Call__ ||
           __Builtin__ == __Name_Builtin_Capacity__ || __Builtin__ == __Name_Builtin_Reserve_Exact__ ||
           __Builtin__ == __Name_Builtin_Pop__ || __Builtin__ == __Name_Builtin_Truncate__ ||
           __Builtin__ == __Name_Builtin_Extend_From__;
}
