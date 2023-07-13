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

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define ROOM_ID      "!koVStwyiiKcBVbXZYz:matrix.org"

void
app_main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);

    MatrixClientSendEvent(&client,
        ROOM_ID,
        "m.room.message",
        "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
        
    MatrixHttpDeinit(&client);
}
