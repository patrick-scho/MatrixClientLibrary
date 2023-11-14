#include <matrix.h>
#include <stdio.h>

#define SERVER        "https://matrix.org"
#define USER_ID       "@example:matrix.org"
#define ROOM_ID       "!example:matrix.org"
#define USERNAME      ""
#define PASSWORD      ""
#define DEVICE_NAME   ""

// device id of another device to share the megolm session with
// I used the device id of a logged in Element web session
#define DEVICE_ID2   ""

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
        DEVICE_ID2,
        megolmOutSession);

    MatrixClientSendEventEncrypted(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
    
    MatrixClientDeleteDevice(&client);

    MatrixHttpDeinit(&client.hc);

    return 0;
}
