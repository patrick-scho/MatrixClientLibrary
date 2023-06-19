#include <stdio.h>
#include <matrix.h>

#define SERVER      "https://matrix.org"
#define USERNAME    "pscho"
#define PASSWORD    "Wc23EbmB9G3faMq"
#define DISPLAYNAME "MatrixClient"


int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientLoginPassword(&client,
        USERNAME,
        PASSWORD,
        DISPLAYNAME);

    printf("Access Token: %s\n", client.accessToken);
    printf("Device ID: %s\n", client.deviceId);
    printf("Expires in (ms): %s\n", client.expireMs);
    printf("Refresh Token: %s\n", client.refreshToken);
    
    MatrixHttpDeinit(&client);

    return 0;
}