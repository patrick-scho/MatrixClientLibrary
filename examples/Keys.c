#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client, SERVER);
    
    MatrixClientSetAccessToken(&client, ACCESS_TOKEN);
    MatrixClientSetDeviceId(&client, DEVICE_ID);
    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientGenerateOnetimeKeys(&client,
        2);
    
    MatrixClientUploadOnetimeKeys(&client);
    MatrixClientUploadDeviceKeys(&client);

    char deviceKey[DEVICE_KEY_SIZE];
    MatrixOlmAccountGetDeviceKey(&client.olmAccount, deviceKey, DEVICE_KEY_SIZE);
    printf("device key: %s\n", deviceKey);
        
    MatrixHttpDeinit(&client);

    return 0;
}
