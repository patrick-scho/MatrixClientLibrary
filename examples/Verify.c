#include <matrix.h>
#include <mjson.h>
#include <olm/sas.h>

#include <stdio.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"

#define DEVICE_ID    "ULZZOKJBYN"
#define SENDER_KEY   "cjP41XzRlY+pd8DoiBuKQJj9o15mrx6gkrpqTkAPZ2c"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"
#define EVENT_ID     "$vOS09eUaI0CduqAcaIU5ZVk6ljLQfLspz7UThP8vaUM"
#define SESSION_ID   "90UbGLue3ADVhvW7hFjoA2c6yg0JJKs/lPdMDZXnZAk"

#define STATIC static

int
main(void)
{
    MatrixClient * client = (MatrixClient*)malloc(sizeof(MatrixClient));
    MatrixClientInit(client);

    MatrixHttpInit(&client->hc, SERVER);
    MatrixClientSetUserId(client, USER_ID);

    MatrixClientLoginPassword(client,
        "pscho",
        "Wc23EbmB9G3faMq",
        "Test1");
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
        "ULZZOKJBYN",
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
