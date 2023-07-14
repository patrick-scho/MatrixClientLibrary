#include <stdio.h>
#include <stdarg.h>

#include <mjson.h>
#include <matrix.h>

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"
#define DEVICE_ID    "MAZNCCZLBR"
#define USER_ID      "@pscho:matrix.org"
#define ROOM_ID      "!XKFUjAsGrSSrpDFIxB:matrix.org"

#define BUFFER_SIZE 1024
#define NUMBER_ARGS 10

void
GetCommand(
    char * cmd,
    int * nargs,
    char ** args
) {
    int index = 0;
    int c;

    *nargs = 0;

    printf("> ");
    while ((c = getchar()), c != ' ' && c != '\n')
        cmd[index++] = c;
    cmd[index] = '\0';

    if (c == '\n')
        return;
    
    *nargs = 1;
    index = 0;
    char * arg = args[0];
    while ((c = getchar()), c != '\n') {
        if (c == ' ') {
            arg[index] = '\0';
            arg = args[(*nargs)++];
            index = 0;
            continue;
        }
        arg[index++] = c;
    }
    arg[index] = '\0';
}

bool
CheckCommand(
    const char * cmd,
    const char * str
) {
    if (strlen(cmd) != strlen(str))
        return false;
    
    for (size_t i = 0; i < strlen(cmd); i++) {
        if (cmd[i] != str[i])
            return false;
    }
    return true;
}

void
Usage(
    const char * cmd,
    const char * args
) {
    printf("Usage: %s %s\n", cmd, args);
}

void
ExecuteCommand(
    MatrixClient * client,
    const char * cmd,
    int nargs, char ** args
) {
    /**/ if (CheckCommand(cmd, "devicekey")) {
        printf("%s\n", client->deviceKey);
    }
    else if (CheckCommand(cmd, "genkeys")) {
        if (nargs != 1) {
            Usage(cmd, "<number of keys>");
            return;
        }
        MatrixClientGenerateOnetimeKeys(client, atoi(args[0]));
    }
    else if (CheckCommand(cmd, "uploadkeys")) {
        MatrixClientUploadOnetimeKeys(client);
    }
    else if (CheckCommand(cmd, "onetimekeys")) {
        static char buffer[1024];
        olm_account_one_time_keys(client->olmAccount.account, buffer, 1024);
        printf("%s\n", buffer);
    }
    else if (CheckCommand(cmd, "getkeys")) {
        MatrixClientRequestDeviceKeys(client);
        for (int i = 0; i < client->numDevices; i++)
            printf("id: %s  key: %s\n",
                client->devices[i].deviceId,
                client->devices[i].deviceKey);
    }
    else if (CheckCommand(cmd, "todevice")) {
        static char buffer[30000];
        MatrixClientSync(client,
            buffer, 30000);
        const char * todevice;
        int todeviceLen;
        mjson_find(buffer, 30000,
            "$.to_device",
            &todevice, &todeviceLen);
        static char prettyBuffer[10000];
        struct mjson_fixedbuf fb = { prettyBuffer, 10000, 0 };
        mjson_pretty(todevice, todeviceLen,
            "  ", mjson_print_fixed_buf, &fb);
        printf("%.*s\n", fb.len, fb.ptr);
    }
}

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);
    MatrixClientSetDeviceId(&client,
        DEVICE_ID);
    MatrixClientSetUserId(&client,
        USER_ID);

    static char cmd[BUFFER_SIZE];
    static char args_[BUFFER_SIZE][NUMBER_ARGS];
    char * args[NUMBER_ARGS];
    for (int i = 0; i < NUMBER_ARGS; i++)
        args[i] = args_[i];
    int nargs;
    do {
        GetCommand(cmd, &nargs, args);

        ExecuteCommand(&client, cmd, nargs, args);
        
    } while (strcmp(cmd, "exit") != 0);
        
    MatrixHttpDeinit(&client);

    return 0;
}
