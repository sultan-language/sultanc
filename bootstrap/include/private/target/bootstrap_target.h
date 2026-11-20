/* Declares the canonical Stage0 bootstrap target model. */

#ifndef SULTANC_BOOTSTRAP_TARGET_BOOTSTRAP_TARGET_H
#define SULTANC_BOOTSTRAP_TARGET_BOOTSTRAP_TARGET_H

#include <stddef.h>

/* Identifies the target architecture facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Architecture_Unknown__,
    __Bootstrap_Target_Architecture_AArch64__,
    __Bootstrap_Target_Architecture_X86_64__,
    __Bootstrap_Target_Architecture_RISCV64__,
    __Bootstrap_Target_Architecture_Wasm32__,
    __Bootstrap_Target_Architecture_Wasm64__
} __Bootstrap_Target_Architecture__;

/* Identifies the target operating-system facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Platform_Unknown__,
    __Bootstrap_Target_Platform_Darwin__,
    __Bootstrap_Target_Platform_Linux__,
    __Bootstrap_Target_Platform_Windows__,
    __Bootstrap_Target_Platform_Wasi__
} __Bootstrap_Target_Platform__;

/* Identifies the target ABI environment facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Environment_Unknown__,
    __Bootstrap_Target_Environment_GNU__,
    __Bootstrap_Target_Environment_Musl__,
    __Bootstrap_Target_Environment_MSVC__
} __Bootstrap_Target_Environment__;

/* Stores one normalized bootstrap compilation target. */
typedef struct
{
    /* Owns the normalized LLVM target triple. */
    char *triple;
    /* Stores the architecture identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Architecture__ architecture;
    /* Stores the operating-system identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Platform__ platform;
    /* Stores the ABI environment identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Environment__ environment;
} __Bootstrap_Target__;

/* Identifies the target-predicate fields Stage0's target model can answer.
 * Spellings mirror compiler/backend/target/target_spelling.sn as consumed by
 * compiler/frontend/parser/target_conditions.sn (قيمة_حقل_حارس_هدف). */
typedef enum
{
    /* الهدف.المنصة */
    __Bootstrap_Target_Condition_Platform__,
    /* الهدف.البيئة */
    __Bootstrap_Target_Condition_Environment__,
    /* الهدف.المعمارية */
    __Bootstrap_Target_Condition_Architecture__,
    /* الهدف.عائلة_المنصة */
    __Bootstrap_Target_Condition_Platform_Family__,
    /* الهدف.صيغة_الكائن */
    __Bootstrap_Target_Condition_Object_Format__,
    /* الهدف.ترتيب_البايت */
    __Bootstrap_Target_Condition_Byte_Order__,
    /* الهدف.عرض_المؤشر */
    __Bootstrap_Target_Condition_Pointer_Width__
} __Bootstrap_Target_Condition_Field__;

/* Initializes one canonical target from an optional Sultan alias or LLVM triple. */
int __Bootstrap_Target_Init__(__Bootstrap_Target__ *target,
                              const char *requested,
                              char *error,
                              size_t error_size);

/* Returns the canonical target-condition spelling of one field for this target,
 * or NULL when the field has no canonical spelling for the classified target.
 * Callers must treat NULL as an invalid target condition, never as a match. */
const char *__Bootstrap_Target_Condition_Value__(const __Bootstrap_Target__ *target,
                                                 __Bootstrap_Target_Condition_Field__ field);

/* Releases one canonical bootstrap target. */
void __Bootstrap_Target_Destroy__(__Bootstrap_Target__ *target);

#endif
