#ifndef MATRIX__H
#define MATRIX__H

#include <stdbool.h>
#include <string.h>

#include <olm/olm.h>





typedef struct FixedBuffer {
    void * ptr;
    int size;
    int len;
} FixedBuffer;

FixedBuffer
FixedBuf(const char * str);



#define ACCESS_TOKEN_LEN 20 // TODO: fix

typedef struct MatrixClient {
    OlmAccount * olmAcc;
    char accessToken[ACCESS_TOKEN_LEN];
} MatrixClient;

bool
MatrixClientInit(
    MatrixClient * client,
    FixedBuffer server
);

bool
MatrixClientLoginPassword(
    MatrixClient * client,
    FixedBuffer username,
    FixedBuffer password
);

bool
MatrixClientGetAccessToken(
    MatrixClient * client,
    FixedBuffer * outBuffer
);

#endif
