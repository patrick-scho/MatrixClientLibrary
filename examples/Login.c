#include <stdio.h>
#include <matrix.h>
#include <mongoose.h>

#define SERVER      "https://matrix.org"
#define USERNAME    "pscho"
#define PASSWORD    "Wc23EbmB9G3faMq"
#define DISPLAYNAME "MatrixClient"


int
main()
{
    MatrixClient client;
    MatrixClientInit(&client, SERVER, strlen(SERVER));
    
    MatrixHttpInit(&client);

    MatrixClientLoginPassword(&client,
        USERNAME, strlen(USERNAME),
        PASSWORD, strlen(PASSWORD),
        DISPLAYNAME, strlen(DISPLAYNAME));

    printf("Access Token: %.*s\n", client.accessTokenLen, client.accessTokenBuffer);
    printf("Device ID: %.*s\n", client.deviceIdLen, client.deviceIdBuffer);
    printf("Expires in (ms): %.*s\n", client.expireMsLen, client.expireMsBuffer);
    printf("Refresh Token: %.*s\n", client.refreshTokenLen, client.refreshTokenBuffer);
    
    MatrixHttpDeinit(&client);

    return 0;
}