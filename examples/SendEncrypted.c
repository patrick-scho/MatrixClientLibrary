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

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);
    MatrixClientSetDeviceId(&client,
        DEVICE_ID);
    MatrixClientSetUserId(&client,
        USER_ID);

    // MatrixMegolmOutSession megolmOutSession;
    // MatrixMegolmOutSessionInit(&megolmOutSession);

    // MatrixClientSetMegolmOutSession(&client,
    //     ROOM_ID,
    //     megolmOutSession);

    MatrixClientSendEventEncrypted(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");

    MatrixClientShareMegolmOutSessionTest(&client,
        "ULZZOKJBYN",
        &client.megolmOutSessions[0]);
        
    MatrixHttpDeinit(&client);

    return 0;
}
