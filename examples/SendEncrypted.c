#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client.hc, SERVER);

    MatrixClientSetUserId(&client, USER_ID);

    MatrixClientLoginPassword(&client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");

    MatrixClientUploadDeviceKeys(&client);
    MatrixClientGenerateOnetimeKeys(&client, 10);
    MatrixClientUploadOnetimeKeys(&client);

    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientNewMegolmOutSession(&client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);

    MatrixClientShareMegolmOutSession(&client,
        USER_ID,
        "ULZZOKJBYN",
        megolmOutSession);

    MatrixClientSendEventEncrypted(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
    
    MatrixClientDeleteDevice(&client);

    MatrixHttpDeinit(&client.hc);

    return 0;
}
