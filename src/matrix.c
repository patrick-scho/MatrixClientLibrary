#include "matrix.h"

FixedBuffer
FixedBuf(const char * str)
{
    int len = strlen(str);
    FixedBuffer result;
    result.ptr = (char *)str;
    result.size = len;
    result.len = len;
    return result;
}


bool
MatrixClientInit(
    MatrixClient * client,
    FixedBuffer server
) {

}

bool
MatrixClientLoginPassword(
    MatrixClient * client,
    FixedBuffer username,
    FixedBuffer password
) {

}

bool
MatrixClientGetAccessToken(
    MatrixClient * client,
    FixedBuffer * outBuffer
) {

}
