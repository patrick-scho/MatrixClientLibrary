#include <stdio.h>
#include <matrix.h>

#define SERVER FixedBuf("matrix.org")
#define USERNAME FixedBuf("@pscho:matrix.org")
#define PASSWORD FixedBuf("abcde")


int
main(
    int argc,
    char **argv)
{
    MatrixClient client;
    MatrixClientInit(&client, SERVER);

    MatrixClientLoginPassword(&client,
        USERNAME,
        PASSWORD);

    static char accessTokenCharBuffer[ACCESS_TOKEN_LEN];
    FixedBuffer accessTokenBuffer = { accessTokenCharBuffer, ACCESS_TOKEN_LEN, 0 };
    MatrixClientGetAccessToken(&client, &accessTokenBuffer);
    printf("Access Token: %.*s\n", accessTokenBuffer.len, (char *)accessTokenBuffer.ptr);

    return 0;
}