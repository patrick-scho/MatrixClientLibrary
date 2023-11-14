#include <mjson.h>
#include <matrix.h>
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
    
    MatrixClientGenerateOnetimeKeys(&client, 10);
    MatrixClientUploadOnetimeKeys(&client);
    MatrixClientUploadDeviceKeys(&client);
    

    static char eventBuffer[1024];
    MatrixClientGetRoomEvent(&client,
        ROOM_ID,
        EVENT_ID,
        eventBuffer, 1024);
    
    printf("event: %s\n", eventBuffer);


    while (getchar() != 'q') {
        static char nextBatch[1024];

        static char syncBuffer[1024*50];
        MatrixClientSync(&client, syncBuffer, 1024*50, nextBatch);
        
        int res;

        const char * s = syncBuffer;
        int slen = strlen(syncBuffer);
        
        {
        int koff, klen, voff, vlen, vtype, off = 0;
        for (off = 0; (off = mjson_next(s, slen, off, &koff, &klen,
                                        &voff, &vlen, &vtype)) != 0; ) {
            const char * key = s + koff;
            const char * val = s + voff;

            printf("%.*s: %.100s\n", klen, key, val);
        }
        }

        mjson_get_string(s, slen, "$.next_batch", nextBatch, 1024);

        const char * events;
        int eventsLen;
        res =
            mjson_find(s, slen, "$.to_device.events", &events, &eventsLen);
        
        if (res != MJSON_TOK_INVALID) {
            {
            int koff, klen, voff, vlen, vtype, off = 0;
            for (off = 0; (off = mjson_next(events, eventsLen, off, &koff, &klen,
                                            &voff, &vlen, &vtype)) != 0; ) {
                const char * val = events + voff;

                printf("%.*s\n", vlen, val);
            }
            }
        }
    }


    MatrixClientDeleteDevice(&client);
        
    MatrixHttpDeinit(&client.hc);

    return 0;
}
