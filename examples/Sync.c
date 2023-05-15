#include <matrix.h>

#define SERVER "matrix.org"
#define ACCESS_TOKEN "abc"
#define ROOM_ID "!jhpZBTbckszblMYjMK:matrix.org"

int
main(
    int argc,
    char **argv)
{
    MatrixClient client;
    MatrixClientCreate(&client,
        SERVER);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);

    static char syncCharBuffer[1024];
    FixedBuffer syncBuffer = { syncCharBuffer, 1024, 0 };
    int syncN = 1;

    while (syncN > 0)
    {
        MatrixClientSyncN(&client, &syncBuffer, &syncN);
        printf("%.*s", syncBuffer.len, (char *)syncBuffer.ptr);
    }
    printf("\n");

    return 0;
}