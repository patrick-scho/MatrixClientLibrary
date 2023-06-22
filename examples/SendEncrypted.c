#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define ROOM_ID      "!koVStwyiiKcBVbXZYz:matrix.org"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);
    MatrixClientSetDeviceId(&client,
        DEVICE_ID);

    // MatrixMegolmOutSession megolmOutSession;
    // MatrixMegolmOutSessionInit(&megolmOutSession);

    // MatrixClientSetMegolmOutSession(&client,
    //     ROOM_ID,
    //     megolmOutSession);

    MatrixClientSendEventEncrypted(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    MatrixHttpDeinit(&client);

    return 0;
}
