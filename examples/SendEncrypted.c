#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

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

    MatrixClientUploadDeviceKey(&client);
    MatrixClientGenerateOnetimeKeys(&client, 10);
    MatrixClientUploadOnetimeKeys(&client);

    // // get device key
    // static char deviceKey[128];
    // MatrixClientGetDeviceKey(&client,
    //     "ULZZOKJBYN",
    //     deviceKey, 128);
    // printf("device key for %s: %s\n", "ULZZOKJBYN", deviceKey);

    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientGetMegolmOutSession(&client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);

    // // create olmsession
    // MatrixOlmSession * olmSession;
    // MatrixClientGetOlmSession(&client,
    //     USER_ID,
    //     "ULZZOKJBYN",
    //     &olmSession);
    // printf("olm session created\n");

    MatrixClientShareMegolmOutSession(&client,
        USER_ID,
        "ULZZOKJBYN",
        megolmOutSession);
    // MatrixClientShareMegolmOutSessionTest(&client,
    //     USER_ID,
    //     "ULZZOKJBYN",
    //     megolmOutSession);

    MatrixClientSendEventEncrypted(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    MatrixClientDeleteDevice(&client);

    MatrixHttpDeinit(&client);

    return 0;
}
