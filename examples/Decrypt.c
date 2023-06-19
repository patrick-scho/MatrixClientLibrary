#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define ROOM_ID      "!koVStwyiiKcBVbXZYz:matrix.org"
#define EVENT_ID     ""

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);

    static char eventBuffer[1024];
    MatrixClientGetRoomEvent(&client,
        ROOM_ID,
        EVENT_ID,
        eventBuffer, 1024);

    MatrixMegolmInSession megolmSession;
    
    MatrixClientRequestMegolmSession(&client,
        ROOM_ID,
        EVENT_ID,
        &megolmSession);

    static char decryptedBuffer[1024];
    MatrixMegolmSessionDecrypt(&megolmSession,
        eventBuffer,
        decryptedBuffer, 1024);

    printf("%s\n", decryptedBuffer);
        
    MatrixHttpDeinit(&client);

    return 0;
}