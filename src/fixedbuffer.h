#ifndef FIXEDBUFFER__H
#define FIXEDBUFFER__H

#include <stdbool.h>

typedef struct FixedBuffer {
    char * ptr;
    int cap;
    int len;
} FixedBuffer;

FixedBuffer
FixedBuf(const char * str);

bool
FixedBufferToInt(FixedBuffer fb, int * outInt);


#endif