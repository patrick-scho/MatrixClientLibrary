#define OLMLIB_VERSION_MAJOR 3
#define OLMLIB_VERSION_MINOR 2
#define OLMLIB_VERSION_PATCH 15

#define OLM_STATIC_DEFINE

#include <stdio.h>
#include <matrix.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

int
main(void)
{
    // static MatrixClient _client;
    // MatrixClient * client = &_client;
    MatrixClient * client = (MatrixClient*)malloc(sizeof(MatrixClient));
    MatrixClientInit(client);

    MatrixHttpInit(&client->hc, SERVER);
    MatrixClientSetUserId(client, USER_ID);

    static char key[1024];
    MatrixOlmAccountGetDeviceKey(&client->olmAccount, key, 1024);
    printf("key: %s\n", key);

    //MatrixClientSetUserId(client, USER_ID);

    MatrixClientLoginPassword(client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");

    // MatrixClientSendEvent(client,
    //     ROOM_ID,
    //     "m.room.message",
    //     "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");

    MatrixClientUploadDeviceKey(client);
    MatrixClientGenerateOnetimeKeys(client, 10);
    MatrixClientUploadOnetimeKeys(client);

    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientNewMegolmOutSession(client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);

    // heap_caps_get_free_size();
    // xPortGetFreeHeapSize();

    MatrixClientShareMegolmOutSession(client,
        USER_ID,
        "ULZZOKJBYN",
        megolmOutSession);

    MatrixClientSendEventEncrypted(client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    MatrixClientDeleteDevice(client);

    MatrixHttpDeinit(&client->hc);

    return 0;
}

#include "wifi.h"

void
app_main(void)
{
    wifi_init("Hundehuette", "Affensicherespw55");

    main();
}
