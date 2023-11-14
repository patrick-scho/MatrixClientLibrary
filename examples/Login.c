#include <stdio.h>
#include <matrix.h>

#define SERVER        "https://matrix.org"
#define USER_ID       "@example:matrix.org"
#define USERNAME      ""
#define PASSWORD      ""
#define DEVICE_NAME   ""


int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client.hc, SERVER);

    MatrixClientLoginPassword(&client,
        USERNAME,
        PASSWORD,
        DEVICE_NAME);

    printf("Access Token: %s\n", client.accessToken);
    printf("Device ID: %s\n", client.deviceId);
    printf("Expires in (ms): %s\n", client.expireMs);
    printf("Refresh Token: %s\n", client.refreshToken);
    
    MatrixHttpDeinit(&client.hc);

    return 0;
}
