/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#define OLMLIB_VERSION_MAJOR 3
#define OLMLIB_VERSION_MINOR 2
#define OLMLIB_VERSION_PATCH 15

#define OLM_STATIC_DEFINE

#include <mongoose.h>
#include <olm/olm.h>
#include <matrix.h>

#include <esp_wifi.h>

#define SERVER       "https://matrix.org"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

void
app_main(void)
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

    // create megolmsession
    MatrixMegolmOutSession * megolmOutSession;
    MatrixClientGetMegolmOutSession(&client,
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

    MatrixHttpDeinit(&client);
}
