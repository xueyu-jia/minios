#ifndef ASSERT_H
#define ASSERT_H

#include "console.h"
#include "string.h"

#define _STR(x) _VAL(x)
#define _VAL(x) #x

#define assert(expr) \
if(!(expr)) do{ \
    disp_color_str("failed:"#expr" ", 0x74);  \
    disp_color_str(__builtin_FILE(), 0x74);  \
    disp_color_str(":"_STR(__LINE__), 0x74);  \
    while(1);   \
}while(0);

#endif