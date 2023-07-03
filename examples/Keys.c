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
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);
    
    MatrixClientSetAccessToken(&client, ACCESS_TOKEN);
    MatrixClientSetDeviceId(&client, DEVICE_ID);
    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientGenerateOnetimeKeys(&client,
        2);
    
    MatrixClientUploadOnetimeKeys(&client);
    MatrixClientUploadDeviceKeys(&client);
        
    MatrixHttpDeinit(&client);

    return 0;
}
