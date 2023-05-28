#include <matrix.h>

#define SERVER       "matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define ROOM_ID      "!jhpZBTbckszblMYjMK:matrix.org"

int
main()
{
    MatrixClient client;
    MatrixClientCreate(&client,
        SERVER, strlen(SERVER));
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN, strlen(ACCESS_TOKEN));

    MatrixClientSendEvent(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");

    return 0;
}