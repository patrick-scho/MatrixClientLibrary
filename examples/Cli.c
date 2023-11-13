#include <stdio.h>
#include <stdarg.h>

#include <mjson.h>
#include <matrix.h>

#define SERVER       "https://matrix.org"
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

bool
ExecuteCommand(
    MatrixClient * client,
    const char * cmd,
    int nargs, char ** args
) {
#define CHECK_ARGS(N, ARGS) if (nargs != N) { Usage(cmd, ARGS); return true; }

    /**/ if (CheckCommand(cmd, "exit")) {
        return false;
    }
    else if (CheckCommand(cmd, "devicekey")) {
        static char key[DEVICE_KEY_SIZE];
        if (MatrixOlmAccountGetDeviceKey(&client->olmAccount, key, DEVICE_KEY_SIZE))
            printf("%s\n", key);
    }
    else if (CheckCommand(cmd, "accesstoken")) {
        printf("%s\n", client->accessToken);
    }
    else if (CheckCommand(cmd, "genkeys")) {
        CHECK_ARGS(1, "<number of keys>")

        MatrixClientGenerateOnetimeKeys(client, atoi(args[0]));
    }
    else if (CheckCommand(cmd, "uploadonetimekeys")) {
        MatrixClientUploadOnetimeKeys(client);
    }
    else if (CheckCommand(cmd, "uploaddevicekey")) {
        MatrixClientUploadDeviceKeys(client);
    }
    else if (CheckCommand(cmd, "onetimekeys")) {
        static char buffer[1024];
        olm_account_one_time_keys(client->olmAccount.account, buffer, 1024);
        printf("%s\n", buffer);
    }
    else if (CheckCommand(cmd, "sendto")) {
        CHECK_ARGS(3, "<device_id> <msgtype> <msg>")

        MatrixClientSendToDevice(client,
            USER_ID,
            args[0],
            args[2],
            args[1]);
    }
    else if (CheckCommand(cmd, "sendtoe")) {
        CHECK_ARGS(3, "<device_id> <msgtype> <msg>")

        MatrixClientSendToDeviceEncrypted(client,
            USER_ID,
            args[0],
            args[2],
            args[1]);
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
            buffer, 30000, "");
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
    else if (CheckCommand(cmd, "login")) {
        CHECK_ARGS(3, "<username> <password> <displayname>")

        MatrixClientLoginPassword(client,
            args[0],
            args[1],
            args[2]);
    }
    else if (CheckCommand(cmd, "save")) {
        CHECK_ARGS(1, "<filename>")

        MatrixClientSave(client, args[0]);
    }
    else if (CheckCommand(cmd, "load")) {
        CHECK_ARGS(1, "<filename>")

        MatrixClientLoad(client, args[0]);
    }
    else if (CheckCommand(cmd, "send")) {
        CHECK_ARGS(2, "<room_id> <message>")

        static char body[1024];
        snprintf(body, 1024,
            "{\"body\":\"%s\",\"msgtype\":\"m.text\"}",
            args[1]);

        MatrixClientSendEvent(client,
            args[0],
            "m.room.message",
            body);
    }
    else if (CheckCommand(cmd, "sendencrypted")) {
        CHECK_ARGS(2, "<room_id> <message>")

        static char body[1024];
        snprintf(body, 1024,
            "{\"body\":\"%s\",\"msgtype\":\"m.text\"}",
            args[1]);

        MatrixClientSendEventEncrypted(client,
            args[0],
            "m.room.message",
            body);
    }
    else if (CheckCommand(cmd, "setuserid")) {
        CHECK_ARGS(1, "<user_id>")

        MatrixClientSetUserId(client, args[0]);
    }
    else if (CheckCommand(cmd, "getuserid")) {
        printf("User ID: %s\n", client->userId);
    }
    else if (CheckCommand(cmd, "sendencrypted")) {
        CHECK_ARGS(2, "<room_id> <message>")

        static char body[1024];
        snprintf(body, 1024,
            "{\"body\":\"%s\",\"msgtype\":\"m.text\"}",
            args[1]);

        MatrixClientSendEventEncrypted(client,
            args[0],
            "m.room.message",
            body);
    }
    else if (CheckCommand(cmd, "sharesession")) {
        CHECK_ARGS(3, "<session_index> <user_id> <device_id>")

        int sessionIndex = atoi(args[0]);

        MatrixClientShareMegolmOutSession(client,
            args[1],
            args[2],
            &client->megolmOutSessions[sessionIndex]);
    }
    else if (CheckCommand(cmd, "savesession")) {
        CHECK_ARGS(3, "<session_index> <filename> <key>")

        int sessionIndex = atoi(args[0]);

        MatrixMegolmOutSessionSave(
            &client->megolmOutSessions[sessionIndex],
            args[1],
            args[2]);
    }
    else if (CheckCommand(cmd, "loadsession")) {
        CHECK_ARGS(3, "<session_index> <filename> <key>")

        int sessionIndex = atoi(args[0]);

        MatrixMegolmOutSessionLoad(
            &client->megolmOutSessions[sessionIndex],
            args[1],
            args[2]);
    }
    else if (CheckCommand(cmd, "printsessions")) {
        for (int i = 0; i < client->numMegolmOutSessions; i++) {
            printf("%d: %s\t%s\t%s\n", i,
                client->megolmOutSessions[i].roomId,
                client->megolmOutSessions[i].id,
                client->megolmOutSessions[i].key);
        }
    }
    else if (CheckCommand(cmd, "initsession")) {
        CHECK_ARGS(1, "<room_id>")

        MatrixMegolmOutSession * megolmOutSession;
        if (! MatrixClientNewMegolmOutSession(client,
            args[0],
            &megolmOutSession))
        {
            printf("Maximum number of Megolm sessions reached (%d)\n", NUM_MEGOLM_SESSIONS);
        }
    }
    
    
    else {
        printf("Unknown command\n");
    }
#undef CHECK_ARGS

    return true;
}

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client);
    
    MatrixHttpInit(&client.hc, SERVER);


    MatrixClientSetUserId(&client, USER_ID);
    MatrixClientLoginPassword(&client, "@pscho:matrix.org", "Wc23EbmB9G3faMq", "abc");
    MatrixClientGenerateOnetimeKeys(&client, 10);
    MatrixClientUploadDeviceKeys(&client);
    MatrixClientUploadOnetimeKeys(&client);


    static char cmd[BUFFER_SIZE];
    static char args_[NUMBER_ARGS][BUFFER_SIZE];
    char * args[NUMBER_ARGS];
    for (int i = 0; i < NUMBER_ARGS; i++)
        args[i] = args_[i];
    int nargs;
    while (1) {
        GetCommand(cmd, &nargs, args);

        bool res =
            ExecuteCommand(&client, cmd, nargs, args);
        
        if (! res)
            break;
    }
    
    MatrixClientDeleteDevice(&client);

    MatrixHttpDeinit(&client.hc);

    return 0;
}
