#include "fixedbuffer.h"

#include <string.h>

FixedBuffer
FixedBuf(const char * str)
{
    int len = strlen(str);
    FixedBuffer result;
    result.ptr = (char *)str;
    result.cap = len;
    result.len = len;
    return result;
}

bool
FixedBufferToInt(FixedBuffer fb, int * outInt)
{
    bool valid = false;
    int result = 0;

    bool negative = false;

    for (int i = 0; i < fb.len; i++)
    {
        if (i == 0 && fb.ptr[i] == '-')
        {
            negative = true;
            continue;
        }

        int val = fb.ptr[i] - '0';
        if (val < 0 || val > 9)
            return false;

        result *= 10;
        result += val;
        valid = true;
    }

    *outInt = result;
    return valid;
}