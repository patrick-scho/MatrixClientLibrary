#include <matrix.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!koVStwyiiKcBVbXZYz:matrix.org"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);


    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientLoginPassword(&client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");


    MatrixClientSendEvent(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    
    MatrixClientDeleteDevice(&client);
        
    MatrixHttpDeinit(&client);

    return 0;
}