#include "frontend/parser/expression_internal.h"

int __Parser_Binary_Info__(__Token_Kind__ __Kind__,
                           int *__Out_Precedence__,
                           __Ast_Binary_Operation__ *__Out_Operation__)
{
    switch (__Kind__)
    {
        case __Token_OR_OR_OPERATOR__:
            *__Out_Precedence__ = 1;
            *__Out_Operation__ = __Binary_Logical_Or__;
            return 1;
        case __Token_AND_AND_OPERATOR__:
            *__Out_Precedence__ = 2;
            *__Out_Operation__ = __Binary_Logical_And__;
            return 1;
        case __Token_OR_OPERATOR__:
            *__Out_Precedence__ = 3;
            *__Out_Operation__ = __Binary_Or__;
            return 1;
        case __Token_CARET_OPERATOR__:
            *__Out_Precedence__ = 4;
            *__Out_Operation__ = __Binary_Xor__;
            return 1;
        case __Token_AND_OPERATOR__:
            *__Out_Precedence__ = 5;
            *__Out_Operation__ = __Binary_And__;
            return 1;
        case __Token_EQUALS_OPERATOR__:
            *__Out_Precedence__ = 6;
            *__Out_Operation__ = __Binary_Equal__;
            return 1;
        case __Token_NOT_EQUAL_OPERATOR__:
            *__Out_Precedence__ = 6;
            *__Out_Operation__ = __Binary_Not_Equal__;
            return 1;
        case __Token_LESS_THAN_OPERATOR__:
            *__Out_Precedence__ = 7;
            *__Out_Operation__ = __Binary_Less_Than__;
            return 1;
        case __Token_LESS_THAN_OR_EQUAL_OPERATOR__:
            *__Out_Precedence__ = 7;
            *__Out_Operation__ = __Binary_Less_Or_Equal__;
            return 1;
        case __Token_GREATER_THAN_OR_EQUAL_OPERATOR__:
            *__Out_Precedence__ = 7;
            *__Out_Operation__ = __Binary_Greater_Or_Equal__;
            return 1;
        case __Token_GREATER_THAN_OPERATOR__:
            *__Out_Precedence__ = 7;
            *__Out_Operation__ = __Binary_Greater_Than__;
            return 1;
        case __Token_LEFT_SHIFT_OPERATOR__:
            *__Out_Precedence__ = 8;
            *__Out_Operation__ = __Binary_Shift_Left_Logical__;
            return 1;
        case __Token_RIGHT_SHIFT_OPERATOR__:
            *__Out_Precedence__ = 8;
            *__Out_Operation__ = __Binary_Shift_Right_Logical__;
            return 1;
        case __Token_PLUS_OPERATOR__:
            *__Out_Precedence__ = 9;
            *__Out_Operation__ = __Binary_Add__;
            return 1;
        case __Token_MINUS_OPERATOR__:
            *__Out_Precedence__ = 9;
            *__Out_Operation__ = __Binary_Subtract__;
            return 1;
        case __Token_STAR_OPERATOR__:
            *__Out_Precedence__ = 10;
            *__Out_Operation__ = __Binary_Multiply__;
            return 1;
        case __Token_SLASH_OPERATOR__:
            *__Out_Precedence__ = 10;
            *__Out_Operation__ = __Binary_Divide__;
            return 1;
        case __Token_PERCENT_OPERATOR__:
            *__Out_Precedence__ = 10;
            *__Out_Operation__ = __Binary_Modulo__;
            return 1;
        default:
            return 0;
    }
}
