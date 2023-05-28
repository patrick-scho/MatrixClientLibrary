#include <stdio.h>
#include <matrix.h>

#define SERVER      "https://matrix.org"
#define USERNAME    "pscho"
#define PASSWORD    "Wc23EbmB9G3faMq"
#define DISPLAYNAME "MatrixClient"


int
main()
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientLoginPassword(&client,
        USERNAME,
        PASSWORD,
        DISPLAYNAME);

    printf("Access Token: %s\n", client.accessTokenBuffer);
    printf("Device ID: %s\n", client.deviceIdBuffer);
    printf("Expires in (ms): %s\n", client.expireMsBuffer);
    printf("Refresh Token: %s\n", client.refreshTokenBuffer);
    
    MatrixHttpDeinit(&client);

    return 0;
}