#include <stdio.h>
#include <matrix.h>

#define SERVER            "https://matrix.org"
#define USER_ID           "@pscho:matrix.org"
#define DEVICE_ID         "BYFEHWFCIV"
#define ACCESS_TOKEN      "syt_cHNjaG8_GgutapSEvKWBQYTqjxQw_2UOtb4"

char ROOM_ID[] =           "!gngUimLZDmREazBCUv:matrix.org";
char TARGET_DEVICE_ID[] =  "ULZZOKJBYN";
char TARGET_DEVICE_KEY[] = "cjP41XzRlY+pd8DoiBuKQJj9o15mrx6gkrpqTkAPZ2c";
char OLM_PICKLE_KEY[] =    "DEFAULT_KEY";
char OLM_ACC_PICKLED[] =   "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";
char OLM_SESS_PICKLED[] =  "zzgNbSTtwx2T1XjDqMuUSbk5aiIUZAsi4emwn8Ooxhxg1TVA73c+p4j9WypfAJjKoJin9cf23k00m1G/6wmISG6WB6KE9FO1jCTYYyDJX+GQxfUVvYwMlavZ9lN2Wax+wxP+3wJ0XZn7aFdjl//I7/lI3kCZGBPnNH+zWjv+19vJyOqz4SGKVTO1ojrds7tA7D7BOT/VxLziQK0XS/k604mWuSJd6BfMmU1bsadSxL2nT0UfhzJckfBkO7q/06twsnIwewKoskv5ZlCo5THoCxinm6aaC/VWozgU7n609xWGtuYdG2C6ijMmAjrfpNFAD9BNu/yqvU/ETfdMDYkhm+RDPlogr2mP";

// session id
// "dBe606wVTSvCSqvSz5pSHtH+f8CvjoB3od6UqOJkssI"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetUserId(&client, USER_ID);
    MatrixClientSetDeviceId(&client, DEVICE_ID);
    MatrixClientSetAccessToken(&client, ACCESS_TOKEN);

    MatrixOlmAccountUnpickle(&client.olmAccount,
        OLM_ACC_PICKLED, strlen(OLM_ACC_PICKLED),
        OLM_PICKLE_KEY, strlen(OLM_PICKLE_KEY));

    // client.numOlmSessions = 1;
    // MatrixOlmSessionUnpickle(&client.olmSessions[0],
    //     TARGET_DEVICE_ID,
    //     OLM_SESS_PICKLED, strlen(OLM_SESS_PICKLED),
    //     OLM_PICKLE_KEY, strlen(OLM_PICKLE_KEY));
    
    // char id[1024];
    // int idLen = olm_session_id(client.olmSessions[0].session, id, 1024);
    // printf("id: %s\n", id, idLen);
    
    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientGetMegolmOutSession(&client,
        ROOM_ID,
        &megolmOutSession);
    printf("megolm session id: %.10s... key: %.10s...\n", megolmOutSession->id, megolmOutSession->key);

    MatrixClientShareMegolmOutSession(&client,
        USER_ID,
        TARGET_DEVICE_ID,
        megolmOutSession);
    
    // MatrixClientSendEventEncrypted(&client,
    //     ROOM_ID,
    //     "m.room.message",
    //     "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");

    MatrixHttpDeinit(&client);

    return 0;
}
