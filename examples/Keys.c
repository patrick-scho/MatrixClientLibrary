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

    MatrixClientGenerateOnetimeKeys(&client,
        10);
    
    MatrixClientUploadDeviceKeys(&client);
    MatrixClientUploadOnetimeKeys(&client);
        
    MatrixHttpDeinit(&client);

    return 0;
}
