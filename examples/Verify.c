#include <matrix.h>
#include <mjson.h>
#include <olm/sas.h>

#include <stdio.h>

#define SERVER        "https://matrix.org"
#define USER_ID       "@example:matrix.org"
#define ROOM_ID       "!example:matrix.org"
#define USERNAME      ""
#define PASSWORD      ""
#define DEVICE_NAME   ""

// event id of an encrypted event
// devices can only be verified after they used e2ee in some way
// (at least in Element)
#define EVENT_ID     "$example"
// device id of another device to share the megolm session with
// I used the device id of a logged in Element web session
#define DEVICE_ID2   ""

#define STATIC static

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
    printf("deviceId: %s\n", client->deviceId);
    MatrixClientGenerateOnetimeKeys(client, 10);
    MatrixClientUploadOnetimeKeys(client);
    MatrixClientUploadDeviceKeys(client);

    STATIC char eventBuffer[1024];
    MatrixClientGetRoomEvent(client,
        ROOM_ID,
        EVENT_ID,
        eventBuffer, 1024);
    printf("event: %s\n", eventBuffer);

    #define SYNC_BUFFER_SIZE 1024*10

    // char * syncBuffer = (char*)malloc(SYNC_BUFFER_SIZE);
    STATIC char syncBuffer[SYNC_BUFFER_SIZE];
    STATIC char nextBatch[1024];

    while (! client->verified) {
        MatrixClientSync(client, syncBuffer, SYNC_BUFFER_SIZE, nextBatch, 1024);
    }

    printf("verified!\n");

    // create and share megolm out session
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientNewMegolmOutSession(client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);
    MatrixClientShareMegolmOutSession(client,
        USER_ID,
        DEVICE_ID2,
        megolmOutSession);

    // send 10 random messages
    for (int i = 0; i < 10; i++) {
        static const char * msgs[] = { "A", "B", "C" };
        static char msg[128];
        snprintf(msg, 128, "{\"body\":\"%s\",\"msgtype\":\"m.text\"}", msgs[rand()%(sizeof(msgs)/sizeof(msgs[0]))]);

        MatrixClientSendEventEncrypted(client,
            ROOM_ID,
            "m.room.message",
            msg);
    }

    MatrixClientDeleteDevice(client);
        
    MatrixHttpDeinit(&client->hc);

    return 0;
}
