#include <matrix.h>

#define SERVER FixedBuf("matrix.org")
#define ACCESS_TOKEN FixedBuf("abc")
#define ROOM_ID FixedBuf("!jhpZBTbckszblMYjMK:matrix.org")

int
main(
    int argc,
    char **argv)
{
    MatrixClient client;
    MatrixClientCreate(&client,
        SERVER);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);

    MatrixClientSendEvent(&client,
        ROOM_ID,
        FixedBuf("m.room.message"),
        FixedBuf("{\"body\":\"Hello\",\"msgtype\":\"m.text\"}"));

    return 0;
}