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
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_فتح_ملف_للقراءة")))
    {
        return __Name_Builtin_Open_File_Read__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قراءة_بايت_ملف")))
    {
        return __Name_Builtin_Read_File_Byte__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قراءة_مقطع_ملف")))
    {
        return __Name_Builtin_Read_File_Segment__;
    }
    if (__Identifier_Identity_Equals__(__Name__,
                                       SULTANC__BUILTIN_NAME__("بدائي_إنشاء_ملف_للكتابة")))
    {
        return __Name_Builtin_Create_File_Write__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_كتابة_مقطع_ملف")))
    {
        return __Name_Builtin_Write_File_Segment__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_إغلاق_ملف")))
    {
        return __Name_Builtin_Close_File__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_فتح_دليل")))
    {
        return __Name_Builtin_Open_Directory__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قراءة_مدخل_دليل")))
    {
        return __Name_Builtin_Read_Directory_Entry__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_إغلاق_دليل")))
    {
        return __Name_Builtin_Close_Directory__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قراءة_بايت_دخل")))
    {
        return __Name_Builtin_Read_Stdin_Byte__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_قراءة_مقطع_دخل")))
    {
        return __Name_Builtin_Read_Stdin_Segment__;
    }
    if (__Identifier_Identity_Equals__(__Name__,
                                       SULTANC__BUILTIN_NAME__("بدائي_كتابة_بايتات_تنفيذية")))
    {
        return __Name_Builtin_Write_Executable_Bytes__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_كتابة_خرج")))
    {
        return __Name_Builtin_Stdout_Write__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_كتابة_خطأ")))
    {
        return __Name_Builtin_Stderr_Write__;
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
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_عدد_الوسائط")))
    {
        return __Name_Builtin_Argument_Count__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_الوسيط")))
    {
        return __Name_Builtin_Argument__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_خروج")))
    {
        return __Name_Builtin_Process_Exit__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نص_من_بايتات")))
    {
        return __Name_Builtin_Text_From_Bytes__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نوع_مسار")))
    {
        return __Name_Builtin_Path_Type__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_حجم_مسار")))
    {
        return __Name_Builtin_Path_Size__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_وقت_تعديل_مسار")))
    {
        return __Name_Builtin_Path_Modified_Time__;
    }
    return __Name_Builtin_None__;
}

/* Checks whether the name builtin is direct source. */
int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__)
{
    return __Builtin__ == __Name_Builtin_Length__ || __Builtin__ == __Name_Builtin_Append__;
}
