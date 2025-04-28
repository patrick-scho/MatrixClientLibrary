# Matrix Client Library in C

This is a library implementing parts of the [Matrix](https://matrix.org/) [Client-Server API](https://spec.matrix.org/v1.8/client-server-api/).
It is written in C and supports sending and receiving of messages, including end-to-end encryption.
Device verification is also supported.

## Building

Building requires a C/C++ compiler and make.

To build the dependencies run `make deps`.
All dependencies are included in this repository.

To build any of the examples run `make out/examples/<example>`.

To use the library:
- Add `src/matrix.c` to compilation
- Add either `src/matrix_http_mongoose.c` or `src/matrix_http_esp32.c` to compilation
- Add `out/*.o` to compilation
- Add include path `src/`
- Add include path `ext/olm/include/`
- Add include path `ext/mjson/src/`
- Add include path `ext/mongoose/`

To build the example for the ESP32 start an ESP-IDF shell in esp32/esp_project or esp32/esp_project_riscv and run:
- `idf.py build`
- `idf.py flash`
- `idf.py monitor`

Examples for the ESP32 are in `esp32/esp_project/main`.
There are currently two, SendEncrypted and Verify.
The example can be set in `esp32/esp_project(_risc_v)/main/CMakeLists.txt` as the second argument after SRCS.

Any code using the library should compile under ESP-IDF if the following code is added at the end of the file:
```c
#include "wifi.h"

void
app_main(void)
{
    wifi_init(WIFI_SSID, WIFI_PASSWORD);

    main();
}

```

To use the library in an ESP-IDF project:
- Add the matrix and olm components (can be found in `esp32/esp_project/components/`)
- Add `wifi.c/.h` (can be found in `esp32/esp_project/main/`)
- Add `SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive")` to CMakeLists.txt
- Call `wifi_init("<SSID>", "<PASSWORD>")` before initializing the library

## Dependencies
[Mongoose](https://github.com/cesanta/mongoose)

[mjson](https://github.com/cesanta/mjson)

[Olm](https://gitlab.matrix.org/matrix-org/olm)

## Examples

### (De)Initialization
```c
MatrixClient * client = (MatrixClient*)malloc(sizeof(MatrixClient));
MatrixClientInit(client);

MatrixHttpInit(&client->hc, SERVER);
MatrixClientSetUserId(client, USER_ID);

MatrixClientLoginPassword(client,
    USERNAME,
    PASSWORD,
    DEVICE_NAME);

MatrixClientDeleteDevice(client);
    
MatrixHttpDeinit(&client->hc);
```

### Uploading keys
```c
MatrixClientGenerateOnetimeKeys(client, 10);
MatrixClientUploadOnetimeKeys(client);
MatrixClientUploadDeviceKeys(client);
```

### Sending an encrypted message
```c
MatrixMegolmOutSession * megolmOutSession;
MatrixClientNewMegolmOutSession(&client,
    ROOM_ID,
    &megolmOutSession);

MatrixClientShareMegolmOutSession(&client,
    USER_ID,
    DEVICE_ID2,
    megolmOutSession);

MatrixClientSendEventEncrypted(&client,
    ROOM_ID,
    "m.room.message",
    "{\"body\":\"Hello\",\"msgtype\":\"m.text\"}");
```

### Verification
```c
// Request an encrypted event to enable verification
STATIC char eventBuffer[1024];
MatrixClientGetRoomEvent(client,
    ROOM_ID,
    EVENT_ID,
    eventBuffer, 1024);

#define SYNC_BUFFER_SIZE 1024*10
STATIC char syncBuffer[SYNC_BUFFER_SIZE];
STATIC char nextBatch[1024];

while (! client->verified) {
    MatrixClientSync(client, syncBuffer, SYNC_BUFFER_SIZE, nextBatch, 1024);
}
```
