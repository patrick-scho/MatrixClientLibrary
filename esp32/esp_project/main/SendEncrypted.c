#define OLMLIB_VERSION_MAJOR 3
#define OLMLIB_VERSION_MINOR 2
#define OLMLIB_VERSION_PATCH 15

#define OLM_STATIC_DEFINE

#include <stdio.h>
#include <matrix.h>

#define SERVER        "https://matrix.org"
#define USER_ID       "@example:matrix.org"
#define ROOM_ID       "!example:matrix.org"
#define USERNAME      ""
#define PASSWORD      ""
#define DEVICE_NAME   ""
#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

int
main(void)
{
    MatrixClient * client = (MatrixClient*)malloc(sizeof(MatrixClient));
    MatrixClientInit(client);
    
    MatrixHttpInit(&client->hc, SERVER);

    MatrixClientSetUserId(client, USER_ID);

    MatrixClientLoginPassword(client,
        USERNAME,
        PASSWORD,
        DEVICE_NAME);

    MatrixClientUploadDeviceKeys(client);
    MatrixClientGenerateOnetimeKeys(client, 10);
    MatrixClientUploadOnetimeKeys(client);

    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientNewMegolmOutSession(client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);

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
    wifi_init(WIFI_SSID, WIFI_PASSWORD);

    main();
}
