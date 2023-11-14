#include <matrix.h>

#define SERVER        "https://matrix.org"
#define USER_ID       "@example:matrix.org"
#define ROOM_ID       "!example:matrix.org"
#define USERNAME      ""
#define PASSWORD      ""
#define DEVICE_NAME   ""

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client.hc, SERVER);


    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientLoginPassword(&client,
        USERNAME,
        PASSWORD,
        DEVICE_NAME);


    MatrixClientSendEvent(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    
    MatrixClientDeleteDevice(&client);
        
    MatrixHttpDeinit(&client.hc);

    return 0;
}
